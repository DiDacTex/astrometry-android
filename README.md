# astrometry-android
This is a fork of [astrometry.net](https://github.com/dstndstn/astrometry.net) modified to run on Android.

For the original README, which includes attributions, license statements, links to the original project, and an academic citation,
please see [README-original.md](https://github.com/DiDacTex/astrometry-android/blob/master/README-original.md).

## Building

The building process for Android is summarized by the `build.sh` script in the repo root.
**However, do not run that script directly!** It needs to be modified for your system.
Follow this process:

1. First, make sure that the Android NDK command-line tools are installed on your system.
1. Export (or set in `build.sh`) the `TOOLCHAIN` environment variable.
   `TOOLCHAIN` should be the absolute path to the tools for your system.
   Example: `/home/jonathan/android/sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/linux-x86_64`
   
   Usually this is expressed in terms of an `NDK` variable, which is the absolute path to the versioned NDK folder.
   Using the above example, `NDK` would be set to `/home/jonathan/android/sdk/ndk/21.1.6352462`.
   `TOOLCHAIN` would then be defined as `$NDK/toolchains/llvm/prebuilt/linux-x86_64`.
1. The only dependency you need is cfitsio. Get it here: https://heasarc.gsfc.nasa.gov/fitsio/
   We tested with 3.48, but later versions are likely to work as well.
   Skip the Android NDK links; as of June 2020, the linked script is extremely outdated.
1. Choose your **minimum** API level and set the `API` variable at the top of the script accordingly.
   Astrometry.net requires API 28 at minimum because it uses the `glob()` and `globfree()` library functions,
   which are not available with lower API levels.
   If you wish to use a lower API level, some people needing those functions for other purposes
   have had success with a modified version of the glob library from BSD, which you can Google for yourself.
1. We have only confirmed that astrometry.net works correctly on ARM64 devices.
   ARM32 produced compilation errors, and x86-64 produced a runtime error for us. x86 was never tested.
   However, if you wish to try architectures other than ARM64,
   list their compiler triplets in the `for` loop line, separated by spaces.
1. Set paths in `build.sh` to point to appropriate directories on your own computer to which you have write access.
1. Build cfitsio (instructions will be added soon).
1. Build astrometry.net (instructions will be added soon).

If all goes right, you will have an installation folder at your specified path.

## Packaging

When targeting API 29 or later, Android enforces W^X ("write XOR execute"),
which denies execute permissions from app-writable directories.
In order to execute native code on API 29+, the code must be executed from the native library directory or from the APK itself.
While actual libraries can execute code from the APK, binary executables cannot.
These executables must be extracted into the library directory on installation.
Unfortunately, the on-device installer filters extracted files
and discards anything whose filename does not start with `lib` and end with `.so`.

Therefore, in order to get the executables onto the device in a place where they can execute, you must rename the files.
We choose to prefix each executable's name with `lib..` and suffix it with `..so`, such as `lib..solve-field..so`.
This "double-dot" convention helps to indicate visually that these are not "normal" libraries, but you do not have to use it.
Depending on which executables you include in your project,
you may need to search the source code and modify it to call them by the new name, then recompile.
(`blind/solve-field.c` has already been modified to call `astrometry-engine` by this double-dotted name convention.)

Many of the executables do not work on Android, because they rely on dependencies such as Python and netpbm
that do not work on Android or require extreme difficulty to get working.
The bare minimum set of executables is just `solve-field` and `astrometry-engine`.
This set requires no source code changes if the latter is renamed to `lib..astrometry-engine..so`,
but can handle only xylists and FITS images.
For JPEG support, download and compile libjpeg-turbo (they have good Android instructions),
then copy `djpeg-static` into your project and rename as needed (suggestion: `lib..djpeg..so`).
Also, add `an-pnmtofits` from astrometry.net.
These two programs will enable you to convert JPEG images to FITS, which can be handled by `solve-field`.

## Running

Assuming you have `solve-field`, `astrometry-engine`, `djpeg`, and `an-pnmtofits`:

To convert a JPEG to FITS: execute the following pipeline (replacing executable names with their renamed versions):
  
`djpeg -grayscale /path/to/jpeg/image | an-pnmtofits > /path/to/FITS/image`

Once you have a FITS image, the following Kotlin call will attempt to solve it:

```kotlin
Runtime.getRuntime().exec(arrayOf(
    "/path/to/solve-field", "--no-plots", "--overwrite", "--no-remove-lines", "--uniformize", "0",
    "--fits-image", "--temp-dir", requireContext().cacheDir.absolutePath
    "--backend-config", "/path/to/config/file", "/path/to/FITS/image"))
```

Brief explanation: Plotting code isn't built, so `--no-plots` disables plotting explicitly.
(Plotting will disable itself if it doesn't find the necessary bits,
but prints an annoying error message unless you explicitly do so.)
`--overwrite` prevents it from failing if you process an image twice.
`--no-remove-lines` and `--uniformize 0` both disable features that require Python.
`--fits-image` prevents astrometry.net from attempting a pointless and
guaranteed-to-fail conversion from FITS to PNM and back to FITS.
`--temp-dir` and its argument point the code at your app's cache directory,
since Android does not have a universal `/tmp` folder.
(If this code is in an activity rather than a fragment, drop `requireContext()`.)
`--backend-config` points the code at the config file,
since it won't be in any location where the code expects to find it.

If you pass it an xylist instead of a FITS image, all of the options remain the same,
except that `--fits-image` should be removed.
