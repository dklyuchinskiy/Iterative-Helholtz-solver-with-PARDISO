#include "definitions.h"
#include "templates.h"
#include "TestSuite.h"

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

#if 0
int main()
{
	TestAll();
	system("pause");
#if 1
	int n1 = 40;		    // number of point across the directions
	int n2 = 40;
	int n3 = 40;
	int n = n1 * n2;		// size of blocks
	int NB = n3;			// number of blocks
	int size = n * NB;		// size of vector x and f: n1 * n2 * n3
	int smallsize = 400;
	double thresh = 1e-6;	// stop level of algorithm by relative error
	int ItRef = 200;		// Maximal number of iterations in refirement
	char bench[255] = "display"; // parameter into solver to show internal results
	int sparse_size = n + 2 * (n - 1) + 2 * (n - n1);
	int non_zeros_in_3diag = n + (n - 1) * 2 + (n - n1) * 2 - (n1 - 1) * 2;

	size_m x, y, z;

	x.n = n1;
	y.n = n2;
	z.n = n3;

	x.l = y.l = z.l = n1 + 1;
	x.h = x.l / (double)(x.n + 1);
	y.h = y.l / (double)(y.n + 1);
	z.h = z.l / (double)(z.n + 1);

	dtype *D;
	dtype *B_mat;

	// Memory allocation for coefficient matrix A
	// the size of matrix A: n^3 * n^3 = n^6
#ifndef ONLINE
	D = alloc_arr(size * n); // it's a matrix with size n^3 * n^2 = size * n
	B_mat = alloc_arr((size - n) * n); 
	int ldd = size;
	int ldb = size - n;
#else
	D = alloc_arr<dtype>(n * n); // it's a matrix with size n^3 * n^2 = size * n
	B_mat = alloc_arr<dtype>(n * n);
	int ldd = n;
	int ldb = n;
#endif

	// Factorization matrix
#ifndef STRUCT_CSR
	double *G = alloc_arr(size * n);
	int ldg = size;
#else
	cmnode **Gstr;
#endif

	// Solution, right hand side and block B
	dtype *B = alloc_arr<dtype>(size - n); // vector of diagonal elementes
	dtype *x_orig = alloc_arr<dtype>(size);
	dtype *x_sol = alloc_arr<dtype>(size);
	dtype *f = alloc_arr<dtype>(size);

#ifdef STRUCT_CSR
	// Memory for CSR matrix
	dcsr *Dcsr;
	int non_zeros_in_block3diag = (n + (n - 1) * 2 + (n - x.n) * 2 - (x.n - 1) * 2) * z.n + 2 * (size - n);
	Dcsr = (dcsr*)malloc(sizeof(dcsr));
	Dcsr->values = alloc_arr<dtype>(non_zeros_in_block3diag);
	Dcsr->ia = alloc_arr<int>(size + 1);
	Dcsr->ja = alloc_arr<int>(non_zeros_in_block3diag);
	Dcsr->ia[size] = non_zeros_in_block3diag + 1;
#endif

	int success = 0;
	int itcount = 0;
	double RelRes = 0;
	double norm = 0;
	int nthr = omp_get_max_threads();
	
	printf("Run in parallel on %d threads\n", nthr);
		
	printf("Grid steps: hx = %lf hy = %lf hz = %lf\n", x.h, y.h, z.h);

#ifndef STRUCT_CSR
	// Generation matrix of coefficients, vector of solution (to compare with obtained) and vector of RHS
	GenMatrixandRHSandSolution(n1, n2, n3, D, ldd, B, x_orig, f);
#else

	// Generation of vector of solution (to compare with obtained), vector of RHS and block B
	GenRHSandSolution(x, y, z, B, x_orig, f);

	// Generation of sparse coefficient matrix
#ifndef ONLINE
	GenSparseMatrix(x, y, z, B_mat, ldb, D, ldd, B_mat, ldb, Dcsr);
#else
	GenSparseMatrixOnline(x, y, z, B_mat, n, D, n, B_mat, n, Dcsr);
	free_arr(D);
#endif
	free_arr(B_mat);

	printf("Non_zeros in block-tridiagonal matrix: %d\n", non_zeros_in_block3diag);

	//	Test_CompareColumnsOfMatrix(n1, n2, n3, D, ldd, B, Dcsr, thresh);
	Test_TransferBlock3Diag_to_CSR(n1, n2, n3, Dcsr, x_orig, f, thresh);
#endif

	printf("Solving %d x %d x %d Laplace equation\n", n1, n2, n3);
	printf("The system has %d diagonal blocks of size %d x %d\n", n3, n1*n2, n1*n2);
	printf("Compressed blocks method\n");
	printf("Parameters: thresh = %g, smallsize = %d \n", thresh, smallsize);

	// Calling the solver
	
#ifndef STRUCT_CSR
	Block3DSPDSolveFast(n1, n2, n3, D, ldd, B, f, thresh, smallsize, ItRef, bench, G, ldg, x_sol, success, RelRes, itcount);
#else

#ifndef ONLINE
	Block3DSPDSolveFastStruct(x, y, z, D, ldd, B, f, Dcsr, thresh, smallsize, ItRef, bench, Gstr, x_sol, success, RelRes, itcount);
#else
	Block3DSPDSolveFastStruct(x, y, z, NULL, ldd, B, f, Dcsr, thresh, smallsize, ItRef, bench, Gstr, x_sol, success, RelRes, itcount);
#endif

#endif
	printf("success = %d, itcount = %d\n", success, itcount);
	printf("-----------------------------------\n");

	printf("Computing error ||x_{exact}-x_{comp}||/||x_{exact}||\n");
	norm = rel_error_complex(n, 1, x_sol, x_orig, size, thresh);

	if (norm < thresh) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, thresh);
	else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, thresh);


#ifdef STRUCT_CSR
	Test_DirFactFastDiagStructOnline(x, y, z, Gstr, B, thresh, smallsize);
	//Test_DirSolveFactDiagStructConvergence(x, y, z, Gstr, thresh, smallsize);
	//Test_DirSolveFactDiagStructBlockRanks(x, y, z, Gstr);

	for (int i = z.n - 1; i >= 0; i--)
		FreeNodes(n, Gstr[i], smallsize);

	free(Gstr);
#endif


#ifndef ONLINE
	free_arr(D);
	free_arr(B);
#endif
	free_arr(x_orig);
	free_arr(x_sol);
	free_arr(f);

	system("pause");

	return 0;
#endif
}

#else

int main()
{
	//TestAll();
	//system("pause");
#if 1
	int n1 = 40;		    // number of point across the directions
	int n2 = 40;
	int n3 = 40;
	int n = n1 * n2;		// size of blocks
	int NB = n3;			// number of blocks
	int size = n * NB;		// size of vector x and f: n1 * n2 * n3
	int smallsize = 1600;
	double thresh = 1e-6;	// stop level of algorithm by relative error
	int ItRef = 200;		// Maximal number of iterations in refirement
	char bench[255] = "display"; // parameter into solver to show internal results
	int sparse_size = n + 2 * (n - 1) + 2 * (n - n1);
	int non_zeros_in_3diag = n + (n - 1) * 2 + (n - n1) * 2 - (n1 - 1) * 2;

	size_m x, y, z;

	x.n = n1;
	y.n = n2;
	z.n = n3;

	x.l = y.l = z.l = n1 + 1;
	x.h = x.l / (double)(x.n + 1);
	y.h = y.l / (double)(y.n + 1);
	z.h = z.l / (double)(z.n + 1);

	dtype *D;
	dtype *B_mat;

	// Memory allocation for coefficient matrix A
	// the size of matrix A: n^3 * n^3 = n^6
#ifndef ONLINE
	D = alloc_arr(size * n); // it's a matrix with size n^3 * n^2 = size * n
	B_mat = alloc_arr((size - n) * n);
	int ldd = size;
	int ldb = size - n;
#else
	D = alloc_arr<dtype>(n * n); // it's a matrix with size n^3 * n^2 = size * n
	B_mat = alloc_arr<dtype>(n * n);
	int ldd = n;
	int ldb = n;
#endif

	// Factorization matrix
#ifndef STRUCT_CSR
	double *G = alloc_arr(size * n);
	int ldg = size;
#else
	cmnode **Gstr;
#endif

	// Solution, right hand side and block B
	dtype *B = alloc_arr<dtype>(size - n); // vector of diagonal elementes
	dtype *x_orig = alloc_arr<dtype>(size);
	dtype *x_sol = alloc_arr<dtype>(size);
	dtype *f = alloc_arr<dtype>(size);
	dtype *f_FFT = alloc_arr<dtype>(size);
	dtype *f2D = alloc_arr<dtype>(n2 * n3);

#ifdef STRUCT_CSR
	// Memory for 3D CSR matrix
	dcsr *Dcsr;
	int non_zeros_in_3Dblock3diag = (n + (n - 1) * 2 + (n - x.n) * 2 - (x.n - 1) * 2) * z.n + 2 * (size - n);
	Dcsr = (dcsr*)malloc(sizeof(dcsr));
	Dcsr->values = alloc_arr<dtype>(non_zeros_in_3Dblock3diag);
	Dcsr->ia = alloc_arr<int>(size + 1);
	Dcsr->ja = alloc_arr<int>(non_zeros_in_3Dblock3diag);
	Dcsr->ia[size] = non_zeros_in_3Dblock3diag + 1;
#endif

	int success = 0;
	int itcount = 0;
	double RelRes = 0;
	double norm = 0;

#ifdef _OPENMP
	int nthr = omp_get_max_threads();
#else
	int nthr = 1;
#endif

	omp_set_dynamic(0);
	omp_set_num_threads(1);

	printf("Run in parallel on %d threads\n", nthr);

	printf("Grid steps: hx = %lf hy = %lf hz = %lf\n", x.h, y.h, z.h);

#ifndef STRUCT_CSR
	// Generation matrix of coefficients, vector of solution (to compare with obtained) and vector of RHS
	GenMatrixandRHSandSolution(n1, n2, n3, D, ldd, B, x_orig, f);
#else
	// Generation of vector of solution (to compare with obtained) and vector of RHS
	GenRHSandSolution(x, y, z, B, x_orig, f);

	// Generation of sparse coefficient matrix
#ifndef ONLINE
	GenSparseMatrix(x, y, z, B_mat, ldb, D, ldd, B_mat, ldb, Dcsr);
#else
	GenSparseMatrixOnline(x, y, z, B_mat, n, D, n, B_mat, n, Dcsr);
//	free_arr(D);
#endif
//	free_arr(B_mat);


	printf("Non_zeros in 3D block-tridiagonal matrix: %d\n", non_zeros_in_3Dblock3diag);

	//	Test_CompareColumnsOfMatrix(n1, n2, n3, D, ldd, B, Dcsr, thresh);
	Test_TransferBlock3Diag_to_CSR(n1, n2, n3, Dcsr, x_orig, f, thresh);

	system("pause");
#endif

	printf("Solving %d x %d x %d Laplace equation using FFT's\n", n1, n2, n3);
	printf("Reduce the problem to set of %d systems of size %d x %d\n", n1, n2*n3, n2*n3);

	// f(x,y,z) -> fy(x,z) 

	DFTI_DESCRIPTOR_HANDLE my_desc1_handle;
	DFTI_DESCRIPTOR_HANDLE my_desc2_handle;
	MKL_LONG status;

	// Create 1D FFT of COMPLEX DOUBLE case
	status = DftiCreateDescriptor(&my_desc1_handle, DFTI_DOUBLE, DFTI_COMPLEX, 1, n1);
	status = DftiSetValue(my_desc1_handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
	status = DftiCommitDescriptor(my_desc1_handle);

	// We make n2 * n3 FFT's for one dimensional direction x with n1 grid points
	printf("Applying 1D Fourier transformation for 3D RHS\n");
	for (int k = 0; k < n2 * n3; k++)
	{
		status = DftiComputeForward(my_desc1_handle, &f[n1 * k], &f_FFT[n1 * k]);
	}

	// Calling the solver
	int size2D = y.n * z.n;
	int mtype = 13;
	int *iparm = alloc_arr<int>(64);
	int *perm = alloc_arr<int>(size2D);
	dtype *x_sol_prd = alloc_arr<dtype>(size);
	size_t *pt = alloc_arr<size_t>(64);

	int maxfct = 1;
	int mnum = 1;
	int phase = 0;
	int rhs = 1;
	int msglvl = 0;
	int error = 0;
	phase = 13;

	// Memory for 2D CSR matrix
	dcsr *D2csr;
	int non_zeros_in_2Dblock3diag = (y.n + (y.n - 1) * 2) * z.n + 2 * (size2D - y.n);
	D2csr = (dcsr*)malloc(sizeof(dcsr));
	D2csr->values = alloc_arr<dtype>(non_zeros_in_2Dblock3diag);
	D2csr->ia = alloc_arr<int>(size2D + 1);
	D2csr->ja = alloc_arr<int>(non_zeros_in_2Dblock3diag);
	D2csr->ia[size2D] = non_zeros_in_2Dblock3diag + 1;

	printf("Non-zeros in 2D block-diagonal: %d\n", non_zeros_in_2Dblock3diag);
	printf("Generating 2D matrix and rhs + solving by pardiso\n");
	for (int i = 0; i < n1; i++)
	{
		printf("Iter: %d\n", i);
		GenSparseMatrixOnline2D(i, y, z, B_mat, n, D, n, B_mat, n, D2csr);
		GenRhs2D(i, x, y, z, f_FFT, f2D);
		pardiso(pt, &maxfct, &mnum, &mtype, &phase, &size2D, D2csr->values, D2csr->ia, D2csr->ja, perm, &rhs, iparm, &msglvl, f2D, &x_sol_prd[i * size2D], &error);
		//printf("Error = %d\n", error);
	}

	printf("Backward 1D FFT's of %d x %d times to each point of 2D solution\n", n2, n3);
	for (int k = 0; k < n2 * n3; k++)
	{
		dtype* u1D = alloc_arr<dtype>(n1);
		GenSol1DBackward(k, x, y, z, x_sol_prd, u1D);
		status = DftiComputeBackward(my_desc1_handle, u1D, &x_sol[k * n1]);
		free(u1D);
	}

	for (int i = 0; i < size; i++)
		x_sol[i] /= n1;

	status = DftiFreeDescriptor(&my_desc1_handle);
	printf("-----------------------------------\n");

	printf("Computing error ||x_{exact}-x_{comp}||/||x_{exact}||\n");

	for (int i = 0; i < size; i++)
		printf("%lf + I * %lf,  %lf + I * %lf\n",x_sol[i].real(), x_sol[i].imag(), x_orig[i].real(), x_orig[i].imag());

	norm = rel_error_complex(n, 1, x_sol, x_orig, size, thresh);

	if (norm < thresh) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, thresh);
	else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, thresh);



#ifndef ONLINE
	free_arr(D);
	free_arr(B);
#endif
	free_arr(x_orig);
	free_arr(x_sol);
	free_arr(f);

	system("pause");

	return 0;
#endif
}
#endif