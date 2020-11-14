#include "templates.h"
#include "TestSuite.h"
#include "TemplatesForMatrixConstruction.h"

/***************************************************
Test for solving Laplace equation with Dirichlet
boundary conditions on the grid n1 x n2 x n3 points 
for the domain x.l x y.l x z.l with HSS technique.

The known solution is generated by function u_ex().
Then we check how correct we has constructed
coefficient matrix as : ||A * u_ex - F_ex|| < eps.

If test is passed, we run the solver for matrix A
and right hand side F.

In the output we compare the relative norm of
the solutuon as:
||u_ex - u_sol|| / ||u_ex|| < eps.

The two version of solver is enabled:
a) with storing the result of factorization in
the array G of doulbe
b) with storing the result of factorization
in the set of structures Gstr, which is defined 
in the definitions.h

The second variant, also, is supported by
storing the initial coefficient matrix A
in sparse CSR format to save memory.

*************************************************/
//#define PRINT
#if 1
int main()
{
//	for (double beta_eq = 0.05; beta_eq <= 1.01; beta_eq += 0.05)
	//	for (int spg_pts = 20; spg_pts <= 100; spg_pts += 20)
		{
			int spg_pts = 50;
			double beta_eq = 0.5;

			printf("*********************************************\n");
			printf("***** beta_eq = %lf spg_pts = %d *******\n", beta_eq, spg_pts);
			printf("*********************************************\n");

#define PERF

#ifndef PERF
			TestAll();
#endif

			//Test2DLaplaceLevander4th(); // laplace + manufactored laplace and helmholtz
			//Test2DHelmholtzLevander4th(); // exact helm
			//Test2DHelmholtzTuning9Pts();
			//TestAll();
#if 0
	//Test2DHelmholtzHODLR();
			system("pause");
			return 0;
#endif

#if 1

#ifdef PML			   // 50 pts   - 7 % and 8 % if beta = 0.3 (ppw = 26)
			//			- 5 % and 6 % if beta = 0.25
			int pml_pts = 20; // 100 pts  - 10 % and 9 % if beta = 0.1
							   //		      6 % and 7 % if beta = 0.2
							   // 150 pts  - 20 % and 10 % if beta = 0.05;
							   //          - 6 % and 3 % if beta = 0.1
							   // 200 pts  - 4 % and 4 % if beta = 0.1, 6 % and ? if beta = 0.2
			//int spg_pts = 25; // 250 pts  - 3 % and 3 % if beta = 0.1

			// 3D
			// 100 pt - 19 % if beta = 0.05
			//			15 % if beta = 0.005
			// 200 pt - 33 % if beta = 0.1
			//		  - 20 % if beta = 0.05
			//		  - 12 % if beta = 0.01
			//		  - 11 % if beta = 0.005
			// 250 pt - 20 % if beta = 0.05
			//        - 10 % if beta = 0.005 - the best (range - 0.08 - 0.01)
			//		  - 11 % if beta = 0.001
			// 1000 pt - 9% if beta = beta = 0.005

			// betas.doc - 28-30 % for ppw = 10 with spg = 250 pts
			//			 - 11% for ppw = 20 with spg = 500 pts
			//		     - 10% with stable z ppw = 20 with spg > 1000
#else
			int pml_pts = 0;
#endif
			int pml_size = 2 * pml_pts;

			size_m x, y, z;
			size_m x_nopml, y_nopml, z_nopml;

			x.pml_pts = y.pml_pts = pml_pts;
			z.pml_pts = 0;

			z.spg_pts = spg_pts;

			x_nopml.pml_pts = y_nopml.pml_pts = z_nopml.pml_pts = 0;

			int n1 = 199 + 2 * x.pml_pts;		    // number of point across the directions
			int n2 = 199 + 2 * y.pml_pts;
			int n3 = 199 + 2 * z.spg_pts;
			int n = n1 * n2;		// size of blocks
			int NB = n3;			// number of blocks

			x.n = n1;
			y.n = n2;
			z.n = n3;

			int size = n * NB;		// size of vector x and f: n1 * n2 * n3
			int size2D = n;
			int smallsize = 1600;
			double thresh = 1e-6;	// stop level of algorithm by relative error
			int ItRef = 200;		// Maximal number of iterations in refirement
			char bench[255] = "display"; // parameter into solver to show internal results
			int sparse_size = n + 2 * (n - 1) + 2 * (n - n1);
			int non_zeros_in_3diag = n + (n - 1) * 2 + (n - n1) * 2 - (n1 - 1) * 2;
			int ione = 1;
			int success = 0;
			int itcount = 0;
			double RelRes = 0;
			double diff_sol = 0;
			double norm = 0;
			bool pml_flag = 1;
			int i1, j1, k1;
			double norm_re, norm_im;

			char str1[255], str2[255], str3[255];


			double timer1, timer2, all_time;

			x.n_nopml = n1 - 2 * x.pml_pts;
			y.n_nopml = n2 - 2 * y.pml_pts;
			z.n_nopml = n3 - 2 * z.spg_pts;

			x_nopml.n = y_nopml.n = x.n_nopml;
			z_nopml.n = z.n_nopml;

			x_nopml.n_nopml = y_nopml.n_nopml = x_nopml.n;
			z_nopml.n_nopml = z_nopml.n;

			x.l = LENGTH_X + (double)(2 * x.pml_pts * LENGTH_X) / (x.n_nopml + 1);
			y.l = LENGTH_Y + (double)(2 * y.pml_pts * LENGTH_Y) / (y.n_nopml + 1);
			z.l = LENGTH_Z + (double)(2 * z.spg_pts * LENGTH_Z) / (z.n_nopml + 1);

			x.h = x.l / (x.n + 1);  // x.n + 1 grid points of the whole domain
			y.h = y.l / (y.n + 1);  // x.n - 1 - inner points
			z.h = z.l / (z.n + 1);  // 2 points - for the boundaries

#ifdef PRINT_INIT
			printf("Size of domain: Lx = %lf, Ly = %lf, Lz = %lf\n", x.l, y.l, z.l);
			printf("with points: Nx = %d, Ny = %d, Nz = %d\n", x.n, y.n, z.n);
			printf("------------------------------\n");
			printf("Size of physical domain: Lx = %lf, Ly = %lf, Lz = %lf\n", x.n_nopml * x.h, y.n_nopml * y.h, z.n_nopml * z.h);
			printf("with points: Nx = %d, Ny = %d, Nz = %d\n", x.n_nopml, y.n_nopml, z.n_nopml);
			printf("Size of PML domain: Lx = %lf, Ly = %lf, Lz = %lf\n", 2 * x.pml_pts * x.h, 2 * y.pml_pts * y.h, 2 * z.pml_pts * z.h);
			printf("with points: Nx = %d, Ny = %d, Nz = %d\n", 2 * x.pml_pts, 2 * y.pml_pts, 2 * z.pml_pts);
			printf("Size of SPNONGE domain: Lz = %lf\n", 2 * z.spg_pts * z.h);
			printf("with points: Nz = %d\n", 2 * z.spg_pts);
			printf("Steps for physical domain: hx = %lf, hy = %lf, hz = %lf\n", x.h, y.h, z.h);

			printf("Size of system Au = f : %d x %d \n", size, size);
#endif
			// ��� ���������� ���� � 2 ������ ������ ����������� � 4 ����!
			// 3D ������
			// ���������� - � ����������x
			// h = 10, 1280 x 1280, N = 120 - 2 �����
			// 40 ����� h = 30, L = 600, omega = 4, 6, 10

#ifndef PERF
			TestHankel();
			system("pause");
#endif

			double lambda = double(c_z) / nu;
			double ppw = lambda / x.h;
			//int niter = NITER * 3;  // 4 iter for freq = 2
							// 12 iter for freq = 4

			//int niter = 25;  // manufactored
			//int niter = 8;// for freq = 4 but n = 50 and BCGStab
							// FGMRES: freq = 4, n = 50, niter = 8

			int niter = 50; // FGMRES 12 (100) 10 (50), BCGSTAB 5 (100), 4 (50)

#ifdef PRINT
			printf("Frequency nu = %d\n", nu);
#endif
#ifdef HOMO
#ifdef PRINT
			printf("The length of the wave: %lf\n", lambda);
			printf("ppw: %lf\n", ppw);
#endif
#else
			printf("Sound speed: min = %lf, max = %lf\n", x.pml_pts * x.h * (C1 + C2) + z.spg_pts * z.h * C3, (x.l - x.pml_pts * x.h) * (C1 + C2) + (z.l - z.spg_pts * z.h) * C3);
#endif


			printf("FGMRES number of iterations: %d\n", niter + 1);


			//system("pause");
			int n_nopml = x.n_nopml * y.n_nopml;
			int size_nopml = n_nopml * z.n_nopml;
			int size2D_nopml = n_nopml;

			//printf("Size no PML: %d\n", size_nopml);

#ifdef PRINT
			printf("-----Memory required:-----\n");
#endif
			double total = 0;
			total = double(size) / (1024 * 1024 * 1024);
			total *= 4 + 2; // 2 for 2D problems - FFT + PARDISO
			total += double(size2D) / (1024 * 1024 * 1024);
			total += double(4 * size_nopml) / (1024 * 1024 * 1024);
			total *= 8;
			total *= 2;

#ifdef PRINT
			printf("Initial = %lf GB\n", total);
#endif

			total = double(size) / (1024 * 1024 * 1024);
			total *= (niter + 1) + 4;
			total += double(6 * size_nopml) / (1024 * 1024 * 1024);
			total *= 8;
			total *= 2;

#ifdef PRINT
			printf("FGMRES = %lf GB\n", total);
#endif

//#define TEST1D

#ifdef TEST1D
			dtype *f1D = alloc_arr<dtype>(x.n);
			dtype *x_sol1D = alloc_arr<dtype>(x.n);
			Solve1DSparseHelmholtz(x, y, z, f1D, x_sol1D, thresh, beta_eq);
#endif

//#define TEST2D

#ifdef TEST2D
			dtype *f2D = alloc_arr<dtype>(x.n * y.n);
			dtype *x_sol2D = alloc_arr<dtype>(x.n * y.n);
			Solve2DSparseHelmholtz(x, y, z, f2D, x_sol2D, thresh, beta_eq);
#endif
			system("pause");
			// Solution and right hand side
			dtype *x_orig = alloc_arr<dtype>(size);
			dtype *x_sol = alloc_arr<dtype>(size);
			dtype *f = alloc_arr<dtype>(size);
			dtype *g = alloc_arr<dtype>(size);

			dtype *x_orig_nopml = alloc_arr<dtype>(size_nopml);
			dtype *x_sol_nopml = alloc_arr<dtype>(size_nopml);
			dtype *f_nopml = alloc_arr<dtype>(size_nopml);
			dtype *g_nopml = alloc_arr<dtype>(size_nopml);

#ifdef TEST_AVE
			stype *x_sol_nopml_direct = alloc_arr<stype>(size_nopml);
			stype *x_anl_nopml_direct = alloc_arr<stype>(size_nopml);
			long int nmodel = (x.n_nopml + 2) * (y.n_nopml + 2) * (z.n_nopml + 2);
			stype *x_sol_nopml_bound_direct = alloc_arr<stype>(nmodel);
			stype *x_anl_nopml_bound_direct = alloc_arr<stype>(nmodel);
			//printf("nmodel = %ld\n", nmodel);
			ReadBinSolution(nmodel, x_sol_nopml_bound_direct, "num51");
			ReadBinSolution(nmodel, x_anl_nopml_bound_direct, "an51");
			DeleteBoundaries<stype>(x.n_nopml + 2, y.n_nopml + 2, z.n_nopml + 2, nmodel, x_sol_nopml_bound_direct, size_nopml, x_sol_nopml_direct);
			DeleteBoundaries<stype>(x.n_nopml + 2, y.n_nopml + 2, z.n_nopml + 2, nmodel, x_anl_nopml_bound_direct, size_nopml, x_anl_nopml_direct);

			pml_flag = true;

			output2D_float("TestCharts2D/model_ft", pml_flag, x, y, x_sol_nopml_direct, x_anl_nopml_direct);
			gnuplot2D("TestCharts2D/model_ft", "TestCharts2D/num", pml_flag, 3, x, y);
			gnuplot2D("TestCharts2D/model_ft", "TestCharts2D/anl", pml_flag, 5, x, y);

			float norm_s = RelError(clange, size_nopml, 1, x_sol_nopml_direct, x_anl_nopml_direct, size_nopml, (float)thresh);
			printf("Relative error Soloviev = %f\n", norm_s);
			//system("pause");

			x_nopml.l = y_nopml.l = z_nopml.l = (double)(LENGTH);
			x_nopml.h = x_nopml.l / (double)(x_nopml.n + 1);  // x.n + 1 grid points of the whole domain
			y_nopml.h = y_nopml.l / (double)(y_nopml.n + 1);  // x.n - 1 - inner points
			z_nopml.h = z_nopml.l / (double)(z_nopml.n + 1);  // 2 points - for the boundaries
#endif

#ifndef PERF
			system("pause");
#endif

#ifdef _OPENMP
			int nthr = omp_get_max_threads();
			printf("Max_threads: %d threads\n", nthr);
			//omp_set_dynamic(0);
			//nthr = 4;
			//omp_set_num_threads(nthr);
			//mkl_set_num_threads(4);
			printf("Run in parallel on %d threads\n", nthr);
#else
			printf("Run sequential version on 1 thread\n");
#endif

#ifdef PRINT
			printf("Grid steps: hx = %lf hy = %lf hz = %lf\n", x.h, y.h, z.h);
#endif
			// Method Runge (for 3 diff grids)
		// f(2h) - f(h)
		// ------------
		// f(h) - f(h/2)

			bool make_runge_count;
			bool make_beta3D_count;

//#define MAKE_RUNGE_3D

#ifdef MAKE_RUNGE_3D
			make_runge_count = true;
#else
			make_runge_count = false;
#endif

			double order;
			if (make_runge_count)
			{
				order = Runge(x, x, x, y, y, y, z, z, z, "sol3D_N50.dat", "sol3D_N100.dat", "sol3D_N200.dat", 3);
				printf("ratio = %lf, order Runge = %lf\n", order, log(order) / log(2.0));
				system("pause");
				return 0;
			}

//#define MAKE_BETA_3D

#ifdef MAKE_BETA_3D
			make_beta3D_count = true;
#else
			make_beta3D_count = false;
#endif


			if (make_beta3D_count)
			{
				size_m x1, y1, z1;
				size_m x2, y2, z2;
				x1.h = x.h;
				y1.h = y.h;
				z1.h = z.h;
				z1.spg_pts = z.spg_pts;
				z2.spg_pts = z.spg_pts;
				
				if (x.n_nopml == 49) order = Beta3D(x1, x2, y1, y2, z1, z2, "sol3D_N50.dat", "sol3D_N100.dat", 3, 50);
				if (x.n_nopml == 99) order = Beta3D(x1, x2, y1, y2, z1, z2, "sol3D_N100.dat", "sol3D_N200.dat", 3, 100);

				system("pause");
				return 0;
			}

			point source = { x.l / 2.0, y.l / 2.0, z.l / 2.0 };

			// Generation of vector of solution (to compare with obtained) and vector of RHS
#ifdef PRINT
			printf("Gen right-hand side and solution...\n");
#endif
			//GenRHSandSolution(x, y, z, x_orig, f, source);

			GenRHSandSolutionViaSound3D(x, y, z, x_orig, f, source);

#endif

			//#define TEST3D

#ifdef TEST3D
			TestFGMRES();
#endif

#ifdef PRINT
			printf("---Residual exact solution---\n");
			ComputeResidual(x, y, z, (double)kk, x_orig, f, g, RelRes);
			printf("-----------\n");
			printf("Residual in 3D with PML |A * x_sol - f| = %e\n", RelRes);
			printf("-----------\n");

			reducePML3D(x, y, z, size, g, size_nopml, g_nopml);

			RelRes = dznrm2(&size_nopml, g_nopml, &ione);
			printf("-----------\n");
			printf("Residual in 3D psys dom  |A * x_sol - f| = %e\n", RelRes);
			printf("-----------\n");
#endif

#ifndef PERF
			system("pause");
#endif

#ifdef OUTPUT
			FILE* out = fopen("ResidualVectorOrig.dat", "w");
			for (int i = 0; i < size; i++)
			{
				take_coord3D(x.n, y.n, z.n, i, i1, j1, k1);
				//	fprintf(out, "%d %d %d %lf %lf\n", i1, j1, k1, f_rsd[i].real(), f_rsd[i].imag());
			}
			fclose(out);
#endif

#ifdef PRINT
			printf("check right-hand-side f\n");
			for (int i = 0; i < size; i++)
				if (abs(f[i]) != 0) printf("f_FFT[%d] = %lf %lf\n", i, f[i].real(), f[i].imag());
#endif
			// ------------ FGMRES-------------
			all_time = omp_get_wtime();

			FGMRES(x, y, z, niter, source, x_sol, x_orig, f, thresh, diff_sol, beta_eq);
			//BCGSTAB(x, y, z, niter, source, x_sol, x_orig, f, thresh, diff_sol, beta_eq);
			// BcgSTAB 

			all_time = omp_get_wtime() - all_time;

			//-------------------------------------------
#ifdef PRINT
			printf("Time: %lf\n", all_time);
			printf("size = %d size_no_pml = %d\n", size, size_nopml);
#endif

			// 4.89e-11 - �� 25 ��������

#ifdef HOMO

#ifndef TEST_HELM_1D
			printf("Nullify orig solution\n");
	// please comment for printing with gnuplot
		    NullifySource2D(x, y, &x_sol[z.n / 2 * size2D], size2D / 2, 1);
			NullifySource2D(x, y, &x_orig[z.n / 2 * size2D], size2D / 2, 1);
#endif

//#define OUTPUT
//#define GNUPLOT

#if 0
			// for printing with gnuplot (1D projections in sponge direction)
			dtype* z_sol1D_ex = alloc_arr<dtype>(z.n);
			dtype* z_sol1D_prd = alloc_arr<dtype>(z.n);

			// output 1D - Z direction
			//char str1[255], str2[255];
			printf("Prepare 1D projections...\n");
			pml_flag = 0;

			for (int j = 0; j < y.n; j++)
			{
				if (j == y.n / 2 || j == y.n / 4 || j == y.n * 3 / 4)
				{
					sprintf(str1, "Charts3D/projZ/model_pml1Dz_sec%d_h%d", j, (int)z.h);
					sprintf(str2, "Charts3D/projZ/model_pml1Dz_diff_sec%d_h%d", j, (int)z.h);

					for (int k = 0; k < z.n; k++)
					{
						z_sol1D_ex[k] = x_orig[j + x.n * j + size2D * k];
						z_sol1D_prd[k] = x_sol[j + x.n * j + size2D * k];
					}

					output1D(str1, pml_flag, z, z_sol1D_ex, z_sol1D_prd);
					gnuplot1D(str1, str2, pml_flag, 0, z);
				}
			}

			printf("Print 1D projections...\n");
			for (int k = 0; k < z.n; k++)
			{
				PrintProjection1D(x, y, &x_orig[k * size2D], &x_sol[k * size2D], k);
			}
#endif

			reducePML3D(x, y, z, size, x_orig, size_nopml, x_orig_nopml);
			reducePML3D(x, y, z, size, x_sol, size_nopml, x_sol_nopml);
			reducePML3D(x, y, z, size, f, size_nopml, f_nopml);

			//	ResidCSR(x_nopml, y_nopml, z_nopml, Dcsr_nopml, x_sol_nopml, f_nopml, g_nopml, RelRes);
			//	printf("-----------\n");
			//	printf("Residual in 3D  ||A * x_sol - f|| = %lf\n", RelRes);
			//	printf("-----------\n");

#ifndef PERF
			system("pause");
#endif
			// Output


			// DEBUG OF RELEASE SETTINGS
			//-------------------
#undef PERF
#define PERF
	// ------------------

#ifndef PERF
#define OUTPUT
#define GNUPLOT
#endif

			printf("----------------------------------------------\n");

			//	free_arr(f);
			//free_arr(g);
			//free_arr(f_nopml);
			//free_arr(g_nopml);

			double *x_orig_re = alloc_arr<double>(size_nopml);
			double *x_sol_re = alloc_arr<double>(size_nopml);
			double *x_orig_im = alloc_arr<double>(size_nopml);
			double *x_sol_im = alloc_arr<double>(size_nopml);

			check_norm_result2(x.n_nopml, y.n_nopml, z.n_nopml, niter, ppw, 2 * z.spg_pts * z.h, x_orig_nopml, x_sol_nopml, x_orig_re, x_orig_im, x_sol_re, x_sol_im);
			compute_and_print_circle_norm(x, y, z, x_orig, x_sol, source, thresh);

			char str[255];
			sprintf(str, "sol3D_N%d.dat", x.n_nopml + 1);
			FILE *file = fopen(str, "w");

			for (int i = 0; i < size_nopml; i++)
				fprintf(file, "%18.16lf %18.16lf\n", x_sol_nopml[i].real(), x_sol_nopml[i].imag());

			fclose(file);

			//printf("Computing error ||x_{exact}-x_{comp_fft}||/||x_{exact}||\n");
#ifndef TEST_HELM_1D
			//check_norm_result2(x.n_nopml, y.n_nopml, z.n_nopml, niter, ppw, 2 * z.spg_pts * z.h, x_orig_nopml, x_sol_nopml, x_orig_re, x_orig_im, x_sol_re, x_sol_im);
			//check_norm_circle(x_nopml, y_nopml, z_nopml, x_orig_nopml, x_sol_nopml, source, thresh);

			//ModifyNumericalSolution(size_nopml, x_sol_nopml, x_orig_nopml);
			//NullifySource2D(x_nopml, y_nopml, &x_sol_nopml_direct[z_nopml.n / 2 * size2D_nopml], size2D_nopml / 2, 1);
#ifdef TEST_AVE
			norm = RelError(zlange, size_nopml, 1, x_sol_nopml, x_orig_nopml, size_nopml, thresh);
			printf("norm_full = %e\n", norm, thresh);

			dtype *x_ave_anl_double = (dtype*)malloc(sizeof(dtype) * size_nopml);
			dtype *x_ave_sol_double = (dtype*)malloc(sizeof(dtype) * size_nopml);

			for (int i = 0; i < size_nopml; i++)
			{
				double real = (double)x_anl_nopml_direct[i].real();
				double imag = (double)x_anl_nopml_direct[i].imag();

				x_ave_anl_double[i] = dtype{ -real, imag };

				real = (double)x_sol_nopml_direct[i].real();
				imag = (double)x_sol_nopml_direct[i].imag();

				x_ave_sol_double[i] = dtype{ -real, imag };
			}

			// nullify ave analyt solution
			NullifySource2D(x_nopml, y_nopml, &x_ave_anl_double[z_nopml.n / 2 * size2D_nopml], size2D_nopml / 2, 1);
			NullifySource2D(x_nopml, y_nopml, &x_ave_sol_double[z_nopml.n / 2 * size2D_nopml], size2D_nopml / 2, 1);

			norm = RelError<dtype>(zlange, size_nopml, 1, x_sol_nopml, x_ave_anl_double, size_nopml, thresh);
			printf("Relative error ME Num vs Soloviev Anl = %f\n", norm);

			norm = RelError<dtype>(zlange, size_nopml, 1, x_sol_nopml, x_orig_nopml, size_nopml, thresh);
			printf("Relative error ME Num vs ME Anl = %f\n", norm);
			
			check_norm_result2(x.n_nopml, y.n_nopml, z.n_nopml, 11, 0, 2 * z.spg_pts * z.h, x_orig_nopml, x_sol_nopml, x_orig_re, x_orig_im, x_sol_re, x_sol_im);

			norm_re = RelError(dlange, size_nopml, 1, x_sol_re, x_orig_re, size_nopml, thresh);
			norm_im = RelError(dlange, size_nopml, 1, x_sol_im, x_orig_im, size_nopml, thresh);
			printf("norm_re = %e\n", norm_re, thresh);
			printf("norm_im = %e\n", norm_im, thresh);

			norm = RelError<dtype>(zlange, size_nopml, 1, x_ave_sol_double, x_orig_nopml, size_nopml, thresh);

			//check_norm_result2(x.n_nopml, y.n_nopml, z.n_nopml, 11, 0, 2 * z.spg_pts * z.h, x_orig_nopml, x_ave_sol_double, x_orig_re, x_orig_im, x_sol_re, x_sol_im);

			printf("Relative error ME Anl vs Soloviev Num = %f\n", norm);

			norm = RelError<dtype>(zlange, size_nopml, 1, x_sol_nopml, x_ave_sol_double, size_nopml, thresh);
			printf("Relative error ME Num vs Soloviev Num = %f\n", norm);

			norm = RelError<dtype>(zlange, size_nopml, 1, x_orig_nopml, x_ave_anl_double, size_nopml, thresh);
			printf("Relative error ME Anl vs Soloviev Anl = %f\n", norm);

			ModifyNumericalSolution(size_nopml, x_sol_nopml, x_orig_nopml);
			norm = RelError(zlange, size_nopml, 1, x_sol_nopml, x_orig_nopml, size_nopml, thresh);
			printf("RelError Adjusted Numerical Solution = %lf\n", norm);
#endif
			//check_norm_result2(x.n_nopml, y.n_nopml, z.n_nopml, 11, 0, 2 * z.spg_pts * z.h, x_orig_nopml, x_ave_anl_double, x_orig_re, x_orig_im, x_sol_re, x_sol_im);

#if 0
	// check analyt solution
			output("TestAnalyt/model_ft", pml_flag, x, y, z, x_orig_nopml, x_ave_anl_double, norm);
			gnuplot("TestAnalyt/model_ft", "TestAnalyt/real/my_ex", pml_flag, 4, x, y, z);
			gnuplot("TestAnalyt/model_ft", "TestAnalyt/imag/my_ex", pml_flag, 5, x, y, z);
			gnuplot("TestAnalyt/model_ft", "TestAnalyt/real/ave_ex", pml_flag, 6, x, y, z);
			gnuplot("TestAnalyt/model_ft", "TestAnalyt/imag/ave_ex", pml_flag, 7, x, y, z);

#endif

			//system("pause");
#if 0
			output2D("TestCharts2D/my_model_ft", pml_flag, x, y, x_sol_nopml, x_orig_nopml);
			gnuplot2D("TestCharts2D/my_model_ft", "TestCharts2D/my_num", pml_flag, 3, x, y);
			gnuplot2D("TestCharts2D/my_model_ft", "TestCharts2D/my_anl", pml_flag, 5, x, y);
#endif
			//system("pause");

#else
			check_test_3Dsolution_in1D(x.n_nopml, y.n_nopml, z.n_nopml, x_sol_nopml, x_orig_nopml, thresh);
#endif

//#define GNUPLOT
//#define OUTPUT

#ifdef OUTPUT
#ifndef HOMO
			norm = diff_sol;
#endif
			pml_flag = true;
			output("ChartsFreq4/model_ft", pml_flag, x, y, z, x_orig_nopml, x_sol_nopml, norm);

			//output("ChartsFreq4_Order4/model_ft", pml_flag, x, y, z, x_orig_nopml, x_sol_nopml);
#endif

//	printf("Computing error ||x_{comp_prd}-x_{comp_fft}||/||x_{comp_prd}||\n");
//	norm = rel_error(zlange, size_nopml, 1, x_pard_nopml_cpy, x_pard_nopml, size_nopml, thresh);

//	if (norm < thresh) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, thresh);
//	else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, thresh);

			free_arr(x_orig_re);
			free_arr(x_orig_im);
			free_arr(x_sol_re);
			free_arr(x_sol_im);

			printf("----------------------------------------------\n");

			//	printf("Test of inverse traversal of the algorithm for NUMERICAL solution\n");
			//	TestInverseTraversal(x, y, z, source, x_sol, f, thresh);
			//	printf("Test of inverse traversal of the algorithm for DIRECT solution\n");
			//	TestInverseTraversal(x, y, z, source, x_orig, f, thresh);

			printf("----------------------------------------------\n");

#ifdef GNUPLOT
			pml_flag = true;
			printf("Print with Gnuplot...\n");

#ifndef ORDER4
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/real/ex_pard", pml_flag, 4, x, y, z);
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/imag/ex_pard", pml_flag, 5, x, y, z);
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/real/helm_ft", pml_flag, 6, x, y, z);
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/imag/helm_ft", pml_flag, 7, x, y, z);
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/real/diff", pml_flag, 8, x, y, z);
			gnuplot("ChartsFreq4/model_ft", "ChartsFreq4/imag/diff", pml_flag, 9, x, y, z);
#else
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/real/ex_pard", pml_flag, 4, x, y, z);
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/imag/ex_pard", pml_flag, 5, x, y, z);
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/real/helm_ft", pml_flag, 6, x, y, z);
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/imag/helm_ft", pml_flag, 7, x, y, z);
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/real/diff", pml_flag, 8, x, y, z);
			gnuplot("ChartsFreq4_Order4/model_ft", "ChartsFreq4_Order4/imag/diff", pml_flag, 9, x, y, z);
#endif
#else
			printf("No printing results...\n");
#endif

#else
	// NON Homogenious domain
	// method Runge

			reducePML3D(x, y, z, size, x_orig, size_nopml, x_orig_nopml);
			reducePML3D(x, y, z, size, x_sol, size_nopml, x_sol_nopml);
			reducePML3D(x, y, z, size, f, size_nopml, f_nopml);

//#define MAKE_RUNGE_3D

#if !defined(MAKE_RUNGE_3D) && !defined(MAKE_BETA_3D)
			RelRes = dznrm2(&size_nopml, x_sol_nopml, &ione);
			printf("norm x_sol = %lf\n", RelRes);

			char str[255];
			sprintf(str, "sol3D_N%d.dat", x.n_nopml + 1);
			FILE *file = fopen(str, "w");

			for (int i = 0; i < size_nopml; i++)
				fprintf(file, "%18.16lf %18.16lf\n", x_sol_nopml[i].real(), x_sol_nopml[i].imag());

			fclose(file);

			//return 0;
#endif

#define MAKE_BETA_3D


#define OUTPUT
#ifdef OUTPUT
			printf("Output results to file...\n");

			char file1[255]; sprintf(file1, "Charts3DHetero/%d/model_ft", x.n_nopml + 1);
			char file2[255]; sprintf(file2, "Charts3DHetero/%d/real/helm_ft", x.n_nopml + 1);
			char file3[255]; sprintf(file3, "Charts3DHetero/%d/imag/helm_ft", x.n_nopml + 1);
			output(file1, pml_flag, x, y, z, x_orig_nopml, x_sol_nopml, diff_sol);
#endif

#define GNUPLOT
#ifdef GNUPLOT
			printf("Printing with GNUPLOT...\n");
			pml_flag = true;
			gnuplot(file1, file2, pml_flag, 6, x, y, z);
			gnuplot(file1, file3, pml_flag, 7, x, y, z);
#else
			printf("No printing results...\n");
#endif


#endif


			free(f);
			free(g);

			free(x_orig_nopml);
			free(x_sol_nopml);
			free(f_nopml);
			free(g_nopml);

#ifdef TEST_AVE

			free(x_ave_anl_double);
			free(x_ave_sol_double);

			free(x_sol_nopml_direct); 
			free(x_anl_nopml_direct);
	
			free(x_sol_nopml_bound_direct);
			free(x_anl_nopml_bound_direct);

			free_arr(x_orig);
			free_arr(x_sol);
#endif

		} // end of tuning loop

	system("pause");
	return 0;
}
#endif

	