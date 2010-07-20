#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "cutest.h"
#include "tweak.h"
#include "sip.h"
#include "sip-utils.h"
#include "log.h"

#define GAUSSIAN_SAMPLE_INVALID -1e300

static double uniform_sample(double low, double high) {
	if (low == high) return low;
	return low + (high - low)*((double)rand() / (double)RAND_MAX);
}

static double gaussian_sample(double mean, double stddev) {
	// from http://www.taygeta.com/random/gaussian.html
	// Algorithm by Dr. Everett (Skip) Carter, Jr.
	static double y2 = GAUSSIAN_SAMPLE_INVALID;
	double x1, x2, w, y1;
	// this algorithm generates random samples in pairs; the INVALID
	// jibba-jabba stores the second value until the next time the
	// function is called.
	if (y2 != GAUSSIAN_SAMPLE_INVALID) {
		y1 = y2;
		y2 = GAUSSIAN_SAMPLE_INVALID;
		return mean + y1 * stddev;
	}
	do {
		x1 = uniform_sample(-1, 1);
		x2 = uniform_sample(-1, 1);
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );
	w = sqrt( (-2.0 * log(w)) / w );
	y1 = x1 * w;
	y2 = x2 * w;
	return mean + y1 * stddev;
}

static void set_grid(int GX, int GY, tan_t* tan, sip_t* sip,
					 double* origxy, double* radec, double* xy,
					 double* gridx, double* gridy) {
	int i, j;

	for (i=0; i<GX; i++)
		gridx[i] = 0.5 + i * tan->imagew / (GX-1);
	for (i=0; i<GY; i++)
		gridy[i] = 0.5 + i * tan->imageh / (GY-1);

	for (i=0; i<GY; i++)
		for (j=0; j<GX; j++) {
			double ra, dec, x, y;
			origxy[2*(i*GX+j) + 0] = gridx[j];
			origxy[2*(i*GX+j) + 1] = gridy[i];
			tan_pixelxy2radec(tan, gridx[j], gridy[i], &ra, &dec);
			radec[2*(i*GX+j) + 0] = ra;
			radec[2*(i*GX+j) + 1] = dec;
			sip_radec2pixelxy(sip, ra, dec, &x, &y);
			xy[2*(i*GX+j) + 0] = x;
			xy[2*(i*GX+j) + 1] = y;
		}

}

static sip_t* run_test(CuTest* tc, sip_t* sip, int N, double* xy, double* radec) {
	int i;
	starxy_t* sxy;
	tweak_t* t;
	sip_t* outsip;
	il* imcorr;
	il* refcorr;
	dl* weights;
	tan_t* tan = &(sip->wcstan);

	printf("Input SIP:\n");
	sip_print_to(sip, stdout);

	sxy = starxy_new(N, FALSE, FALSE);
	starxy_set_xy_array(sxy, xy);

	imcorr = il_new(256);
	refcorr = il_new(256);
	weights = dl_new(256);
	for (i=0; i<N; i++) {
		il_append(imcorr, i);
		il_append(refcorr, i);
		dl_append(weights, 1.0);
	}

	t = tweak_new();
	tweak_push_wcs_tan(t, tan);

	outsip = t->sip;
	outsip->a_order = outsip->b_order = sip->a_order;
	outsip->ap_order = outsip->bp_order = sip->ap_order;

	t->weighted_fit = TRUE;
	tweak_push_ref_ad_array(t, radec, N);
	tweak_push_image_xy(t, sxy);
	tweak_push_correspondence_indices(t, imcorr, refcorr, NULL, weights);
	tweak_skip_shift(t);

	// push correspondences
	// push image xy
	// push ref ra,dec
	// push ref xy (tan)
	// push tan

	tweak_go_to(t, TWEAK_HAS_LINEAR_CD);

	printf("Output SIP:\n");
	sip_print_to(outsip, stdout);

	CuAssertDblEquals(tc, tan->imagew, outsip->wcstan.imagew, 1e-10);
	CuAssertDblEquals(tc, tan->imageh, outsip->wcstan.imageh, 1e-10);

	// should be exactly equal.
	CuAssertDblEquals(tc, tan->crpix[0], outsip->wcstan.crpix[0], 1e-10);
	CuAssertDblEquals(tc, tan->crpix[1], outsip->wcstan.crpix[1], 1e-10);

	t->sip = NULL;
	tweak_free(t);
	starxy_free(sxy);
	return outsip;
}
					 
void test_tweak_1(CuTest* tc) {
	int GX = 5;
	int GY = 5;
	double origxy[GX*GY*2];
	double xy[GX*GY*2];
	double radec[GX*GY*2];
	double gridx[GX];
	double gridy[GY];
	sip_t thesip;
	sip_t* sip = &thesip;
	tan_t* tan = &(sip->wcstan);
	int i;
	sip_t* outsip;

	log_init(LOG_VERB);

	memset(sip, 0, sizeof(sip_t));

	tan->imagew = 2000;
	tan->imageh = 2000;
	tan->crval[0] = 150;
	tan->crval[1] = -30;
	tan->crpix[0] = 1000.5;
	tan->crpix[1] = 1000.5;
	tan->cd[0][0] = 1./1000.;
	tan->cd[0][1] = 0;
	tan->cd[1][1] = 1./1000.;
	tan->cd[1][0] = 0;

	sip->a_order = sip->b_order = 2;
	sip->a[2][0] = 10.*1e-6;
	sip->ap_order = sip->bp_order = 4;

	sip_compute_inverse_polynomials(sip, 0, 0, 0, 0, 0, 0);

	set_grid(GX, GY, tan, sip, origxy, radec, xy, gridx, gridy);

	outsip = run_test(tc, sip, GX*GY, xy, radec);

	CuAssertDblEquals(tc, tan->crval[0], outsip->wcstan.crval[0], 1e-6);
	CuAssertDblEquals(tc, tan->crval[1], outsip->wcstan.crval[1], 1e-6);

	CuAssertDblEquals(tc, tan->cd[0][0], outsip->wcstan.cd[0][0], 1e-10);
	CuAssertDblEquals(tc, tan->cd[0][1], outsip->wcstan.cd[0][1], 1e-10);
	CuAssertDblEquals(tc, tan->cd[1][0], outsip->wcstan.cd[1][0], 1e-10);
	CuAssertDblEquals(tc, tan->cd[1][1], outsip->wcstan.cd[1][1], 1e-10);

	double *d1, *d2;
	d1 = (double*)outsip->a;
	d2 = (double*)&(sip->a);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-10);
	d1 = (double*)outsip->b;
	d2 = (double*)&(sip->b);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-10);
	d1 = (double*)outsip->ap;
	d2 = (double*)&(sip->ap);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-10);
	d1 = (double*)outsip->bp;
	d2 = (double*)&(sip->bp);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-10);
	CuAssertIntEquals(tc, sip->a_order, outsip->a_order);
	CuAssertIntEquals(tc, sip->b_order, outsip->b_order);
	CuAssertIntEquals(tc, sip->ap_order, outsip->ap_order);
	CuAssertIntEquals(tc, sip->bp_order, outsip->bp_order);
}


void test_tweak_2(CuTest* tc) {
	int GX = 11;
	int GY = 11;
	double origxy[GX*GY*2];
	double xy[GX*GY*2];
	double noisyxy[GX*GY*2];
	double radec[GX*GY*2];
	double gridx[GX];
	double gridy[GY];
	sip_t thesip;
	sip_t* sip = &thesip;
	tan_t* tan = &(sip->wcstan);
	int i,j;
	sip_t* outsip;

	log_init(LOG_VERB);

	memset(sip, 0, sizeof(sip_t));

	tan->imagew = 2000;
	tan->imageh = 2000;
	tan->crval[0] = 150;
	tan->crval[1] = -30;
	tan->crpix[0] = 1000.5;
	tan->crpix[1] = 1000.5;
	tan->cd[0][0] = 1./1000.;
	tan->cd[0][1] = 0;
	tan->cd[1][1] = 1./1000.;
	tan->cd[1][0] = 0;

	sip->a_order = sip->b_order = 2;
	sip->a[2][0] = 10.*1e-6;
	sip->b[0][2] = -10.*1e-6;
	sip->ap_order = sip->bp_order = 4;

	sip_compute_inverse_polynomials(sip, 0, 0, 0, 0, 0, 0);

	set_grid(GX, GY, tan, sip, origxy, radec, xy, gridx, gridy);

	// add noise to observed xy positions.
	srand(42);
	for (i=0; i<(GX*GY*2); i++) {
		noisyxy[i] = xy[i] + gaussian_sample(0.0, 1.0);
	}

	outsip = run_test(tc, sip, GX*GY, noisyxy, radec);

	fprintf(stderr, "from numpy import array\n");
	fprintf(stderr, "x0,y0 = %g,%g\n", tan->crpix[0], tan->crpix[1]);
	fprintf(stderr, "gridx=array([");
	for (i=0; i<GX; i++)
		fprintf(stderr, "%g, ", gridx[i]);
	fprintf(stderr, "])\n");
	fprintf(stderr, "gridy=array([");
	for (i=0; i<GY; i++)
		fprintf(stderr, "%g, ", gridy[i]);
	fprintf(stderr, "])\n");
	fprintf(stderr, "origxy_2 = array([");
	for (i=0; i<(GX*GY); i++)
		fprintf(stderr, "[%g,%g],", origxy[2*i+0], origxy[2*i+1]);
	fprintf(stderr, "])\n");
	fprintf(stderr, "xy_2 = array([");
	for (i=0; i<(GX*GY); i++)
		fprintf(stderr, "[%g,%g],", xy[2*i+0], xy[2*i+1]);
	fprintf(stderr, "])\n");

	fprintf(stderr, "noisyxy_2 = array([");
	for (i=0; i<(GX*GY); i++)
		fprintf(stderr, "[%g,%g],", noisyxy[2*i+0], noisyxy[2*i+1]);
	fprintf(stderr, "])\n");

	fprintf(stderr, "truesip_a_2 = array([");
	for (i=0; i<=sip->a_order; i++)
		for (j=0; j<=sip->a_order; j++)
			if (sip->a[i][j] != 0)
				fprintf(stderr, "[%i,%i,%g],", i, j, sip->a[i][j]);
	fprintf(stderr, "])\n");

	fprintf(stderr, "truesip_b_2 = array([");
	for (i=0; i<=sip->a_order; i++)
		for (j=0; j<=sip->a_order; j++)
			if (sip->b[i][j] != 0)
				fprintf(stderr, "[%i,%i,%g],", i, j, sip->b[i][j]);
	fprintf(stderr, "])\n");

	fprintf(stderr, "sip_a_2 = array([");
	for (i=0; i<=outsip->a_order; i++)
		for (j=0; j<=outsip->a_order; j++)
			if (outsip->a[i][j] != 0)
				fprintf(stderr, "[%i,%i,%g],", i, j, outsip->a[i][j]);
	fprintf(stderr, "])\n");

	fprintf(stderr, "sip_b_2 = array([");
	for (i=0; i<=outsip->a_order; i++)
		for (j=0; j<=outsip->a_order; j++)
			if (outsip->b[i][j] != 0)
				fprintf(stderr, "[%i,%i,%g],", i, j, outsip->b[i][j]);
	fprintf(stderr, "])\n");

	CuAssertDblEquals(tc, tan->crval[0], outsip->wcstan.crval[0], 1e-3);
	CuAssertDblEquals(tc, tan->crval[1], outsip->wcstan.crval[1], 1e-3);

	CuAssertDblEquals(tc, tan->cd[0][0], outsip->wcstan.cd[0][0], 1e-6);
	CuAssertDblEquals(tc, tan->cd[0][1], outsip->wcstan.cd[0][1], 1e-6);
	CuAssertDblEquals(tc, tan->cd[1][0], outsip->wcstan.cd[1][0], 1e-6);
	CuAssertDblEquals(tc, tan->cd[1][1], outsip->wcstan.cd[1][1], 1e-6);

	double *d1, *d2;
	d1 = (double*)outsip->a;
	d2 = (double*)&(sip->a);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		// rather large error, no?
		CuAssertDblEquals(tc, d2[i], d1[i], 6e-7);
	d1 = (double*)outsip->b;
	d2 = (double*)&(sip->b);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 2e-7);
	d1 = (double*)outsip->ap;
	d2 = (double*)&(sip->ap);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-6);
	d1 = (double*)outsip->bp;
	d2 = (double*)&(sip->bp);
	for (i=0; i<(SIP_MAXORDER * SIP_MAXORDER); i++)
		CuAssertDblEquals(tc, d2[i], d1[i], 1e-6);
	CuAssertIntEquals(tc, sip->a_order, outsip->a_order);
	CuAssertIntEquals(tc, sip->b_order, outsip->b_order);
	CuAssertIntEquals(tc, sip->ap_order, outsip->ap_order);
	CuAssertIntEquals(tc, sip->bp_order, outsip->bp_order);

}



void test_tchebytweak(CuTest* tc) {


}

