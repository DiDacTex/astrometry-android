# astrometry-android
This is a fork of [astrometry.net](https://github.com/dstndstn/astrometry.net) modified to run on Android as a JNI library.
As presented here, Chaquopy is used to run some Python portions of the astrometry.net suite.

For the original README, which includes attributions, license statements, links to the original project, and an academic citation,
please see [README-original.md](https://github.com/DiDacTex/astrometry-android/blob/master/README-original.md).

## Building

Normally, astrometry.net builds a set of 40+ executables (mostly binary, some Python).
However, Android [is not designed to play nice with native executables](https://stackoverflow.com/questions/16179062/using-exec-with-ndk).
Therefore, we have modified `solve-field` and `astrometry-engine` to instead compile as a library
that you can call into using the Java Native Interface, which is the recommended way to use native code on Android.
At this time, only `solve-field` and `astrometry-engine` are compiled into the library; the rest of the programs are not.
However, those two are the core programs you need to solve images of the sky, which is the primary reason for using astrometry.net.

The building process for Android is summarized by the `build.sh` script in the repo root.
**However, do not run that script directly!** It needs some preparation.
Follow this process:

1. First, make sure that the Android NDK command-line tools are installed on your system.
1. Export (or set in `build.sh`) the `TOOLCHAIN` environment variable.
   `TOOLCHAIN` should be the absolute path to the tools for your system.
   Example from my system: `/home/jonathan/android/sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/linux-x86_64`
   
   Usually this is expressed in terms of an `NDK` variable, which is the absolute path to the versioned NDK folder.
   Using the above example, `NDK` would be set to `/home/jonathan/android/sdk/ndk/21.1.6352462`.
   `TOOLCHAIN` would then be defined as `$NDK/toolchains/llvm/prebuilt/linux-x86_64`.
1. The only external dependency you need is cfitsio. Get it here: https://heasarc.gsfc.nasa.gov/fitsio/
   We tested with 3.48, but later versions are likely to work as well.
   Skip the Android NDK links; as of June 2020, the linked script is extremely outdated.
   Unpack the tarball into a folder inside the root of this repo, and rename the folder to `cfitsio`.
   (Make sure you use exactly this name, because the build script expects it.)
1. Choose your **minimum** API level (*not* target API) and set the `API` variable at the top of the script accordingly.
   Astrometry.net requires at least API 28 because it uses the `glob()` and `globfree()` library functions,
   which are not available with lower API levels.
   If you wish to use a lower API level, some people needing those functions for other purposes
   have had success with a modified version of the glob library from BSD, which you can Google for yourself.
1. We have only confirmed that astrometry.net works correctly on ARM64 devices.
   ARM32 produced compilation errors, and x86-64 produced a runtime error for us. We have not attempted x86.
   However, if you wish to try architectures other than ARM64,
   list their ABI names in the `for` loop line, separated by spaces.
1. Run the command `./build.sh`. For each ABI you selected above, the script will build cfitsio,
   then astrometry.net, cleaning both directories afterward to prepare for the next ABI.
   It will then assemble the minimal files you need for Android use.

If all goes right, you will have an `build` folder in this directory, containing one folder for each ABI,
each containing three folders: `cfitsio`, `astrometry`, and `android`.
The first two contain the "installation" of those packages, which you can ignore.
`android` contains the library to be copied into your Gradle project; more on that below.

## Packaging

Copy the folders from the `build/android` folder and place them in your project's `main` folder, alongside the `AndroidManifest.xml`.
If the `assets`, `jniLibs`, or `python` folders already exist in `main`, merge the contents.
Be sure to keep the ABI subfolder(s) inside `jniLibs`.
Note that the `python` folder assumes Chaquopy; if you are using a different distribution, adjustments may be necessary.

You also need index files. For smartphone photography, we recommend [these wide-angle indexes](http://data.astrometry.net/4100/).
Indexes 4115 through 4119 are probably a good set for smartphones, assuming that your users are merely pointing the camera at the sky.
They may not work as well if the camera is being zoomed or held up to a telescope.
Place these index files in your project's `assets` folder (or a subfolder of that folder).

On app launch, you will need to copy the index files from the assets to your app's internal (or external) files directory,
as well as write a backend config file.
The latter is not provided by this build because it needs to include the path to your index directory on the phone's storage medium,
after the files have been copied out of assets. A suggested config file looks something like this:

```
inparallel
cpulimit 300
add_path /path/to/index/files
autoindex
```

Adjust the CPU limit as you see fit, and change the path to point to your index files
(best to query the files directory from the Android API).

Finally, we do not yet directly provide the Java code needed to interface with the library and run Python scripts.
Fortunately, it's simple.
Within your project's `java` folder, create subfolders for the package `net.astrometry`,
and place a single file in that folder named `JNI.java`.
In that file, copy and paste the following code, assuming you are using Chaquopy for Python:

```java
package net.astrometry;

import com.chaquo.python.Kwarg;
import com.chaquo.python.PyObject;
import com.chaquo.python.Python;

public class JNI {
    static Python python = Python.getInstance();

    public static native int solveField(String[] args, double[] results);

    // This function is called from libastrometry.so via JNI
    public static void removelines(String infile, String outfile, String xcol, String ycol) {
        PyObject module = python.getModule("astrometry.util.removelines");
        module.callAttr("removelines", infile, outfile, new Kwarg("xcol", xcol), new Kwarg("ycol", ycol));
    }

    // This function is called from libastrometry.so via JNI
    public static void uniformize(String infile, String outfile, int n, String xcol, String ycol) {
        PyObject module = python.getModule("astrometry.util.uniformize");
        module.callAttr("uniformize", infile, outfile, n, new Kwarg("xcol", xcol), new Kwarg("ycol", ycol));
    }
}
```

Note for Chaquopy: you will need to follow the documentation to use pip to install `numpy` and `pyfits`.
`pyfits` will NOT install from PyPI;
you will need to clone the GitHub repo, check out the 3.4 tag, delete all C files,
and remove them and the `numpy_extension_hook` from `setup.cfg`,
then build an sdist or a wheel and place it in your source tree for local installation.

If you are using a different Python implementation,
you will need to rewrite the body of the `removelines` and `uniformize` methods
and install `numpy` and `pyfits` according to that distribution's documentation.

Once you've completed these steps, you're ready to use the library in your project.

## Running

Assuming you are using the library as provided here, you will need a FITS image or an xylist.
[The FITS format specification is freely available](https://fits.gsfc.nasa.gov/fits_documentation.html),
and it is not a difficult format to produce by hand if you have uncompressed image data (e.g. from a fresh capture).
Some FITS libraries are available, but are not needed for simple files, and you can easily roll your own.

Once you have a FITS image, you can call this library's sole JNI function, `solveField`, which takes two arguments:
an input array of `String`s that represent the arguments that you would pass to the `solve-field` executable, and
an output array of `double`s of size 2, which will contain the celestial coordinates when the function returns.
The function returns zero if it completes normally and non-zero in case of problems.

A recommended way to call the function in Kotlin is:

```kotlin
val results = DoubleArray(2)
val args = arrayOf(
    "--no-plots", "--overwrite",
    "--fits-image", "--temp-dir", requireContext().cacheDir.absolutePath
    "--backend-config", "/path/to/config/file", "/path/to/FITS/image")
val code = JNI.solveField(args, results)
if (code == 0) {
    // interpret results
}
```

Brief explanation:
Plotting code isn't built, so `--no-plots` disables plotting explicitly.
(Plotting will disable itself if it doesn't find the necessary bits,
but outputs a useless error message unless you explicitly disable it.)
`--overwrite` prevents it from failing if you process an image twice.
`--fits-image` prevents astrometry.net from attempting a pointless and
guaranteed-to-fail conversion from FITS to PNM and back to FITS.
`--temp-dir` and its argument point the code at your app's cache directory,
since Android does not have a universal `/tmp` folder.
(If this code is in an activity rather than a fragment, drop `requireContext()`.)
`--backend-config` points the code at the config file,
since it won't be in any location where the code expects to find it.

If you pass it an xylist instead of a FITS image, all of the options remain the same,
except that `--fits-image` should be removed.

All output from the library will be sent to logcat.
Upon return, the first two elements of `results` will be set according to the solution for the image.
If the image was solved, `results[0]` will contain the right ascension of the center of the image,
and `results[1]` will contain the declination of the center of the image. Both will be in decimal degrees.
If the image failed to solve, both elements will contain values greater than 360.0 to signal this.
Therefore you can check for a failed solving attempt by checking one element for that condition.
(Internally, the signal value for an unsolved image is currently 1000.0,
but you should not depend on a specific value. Rather, check whether the value is higher or lower than 360.0.)
Note that it is possible for `solveField` to raise an exception if a memory allocation fails;
in that case, the `results` array will not have been modified in the JNI function,
and if you have not initialized it yourself, it may contain junk values.
