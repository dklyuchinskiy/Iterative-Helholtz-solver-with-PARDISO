#pragma once

template <typename MatrixType>
void Diag(int n, MatrixType *H, int ldh, MatrixType value)
{
	for (int j = 0; j < n; j++)
		for (int i = 0; i < n; i++)
			if (j == i) H[i + ldh * j] = value;
			else H[i + ldh * j] = 0.0;
}

template <typename MatrixType>
map<vector<int>, MatrixType> dense_to_CSR(int m, int n, MatrixType *A, int lda, int *ia, int *ja, MatrixType *values)
{
	map<vector<int>, MatrixType> CSR;
	vector<int> v(2, 0);
	int k = 0;
	int ik = 0;
	int first_elem_in_row = 0;
	//print(m, n, A, lda, "Arow[0]");
	for (int i = 0; i < m; i++)
	{
		first_elem_in_row = 0;
		for (int j = 0; j < n; j++)
		{
			if (abs(A[i + lda * j]) != 0)
			{
				values[k] = A[i + lda * j];
			//	printf("%lf %lf\n", A[i + lda * j].real(), A[i + lda * j].imag());
				if (first_elem_in_row == 0)
				{
					ia[ik] = k + 1;
					ik++;
					first_elem_in_row = 1;
				}
				ja[k] = j + 1;

				v[0] = ia[ik - 1];
				v[1] = ja[k];
				CSR[v] = values[k];

				k++;
			}
		}
	}

	return CSR;
}

template <typename MatrixType>
double rel_error(double (*LANGE)(const char *, const int*, const int*, const MatrixType*, const int*, double *),
	int n, int k, MatrixType *Hrec, MatrixType *Hinit, int ldh, double eps)
{
	double norm = 0;

	// Norm of residual
#pragma omp parallel for schedule(static)
	for (int j = 0; j < k; j++)
//#pragma omp simd
		for (int i = 0; i < n; i++)
			Hrec[i + ldh * j] = Hrec[i + ldh * j] - Hinit[i + ldh * j];

	norm = LANGE("Frob", &n, &k, Hrec, &ldh, NULL);
	norm = norm / LANGE("Frob", &n, &k, Hinit, &ldh, NULL);

	return norm;

	//if (norm < eps) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, eps);
	//else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, eps);
}

template <typename MatrixType, typename RealType>
RealType RelError(RealType(*LANGE)(const char *, const int*, const int*, const MatrixType*, const int*, RealType *),
	int m, int n, const MatrixType *Hrec, const MatrixType *Hinit, int ldh, RealType eps)
{
	RealType norm = 0;
	MatrixType *Hdiff = alloc_arr<MatrixType>(m * n);

	// Norm of residual
	for (int j = 0; j < n; j++)
#pragma omp simd
		for (int i = 0; i < m; i++)
			Hdiff[i + ldh * j] = Hrec[i + ldh * j] - Hinit[i + ldh * j];

	norm = LANGE("Frob", &m, &n, Hdiff, &ldh, NULL);
	norm = norm / LANGE("Frob", &m, &n, Hinit, &ldh, NULL);

	free_arr(Hdiff);

	return norm;

	//if (norm < eps) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, eps);
	//else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, eps);
}

template <typename MatrixType, typename RealType>
RealType RelErrorNorm1(RealType(*LANGE)(const char*, const int*, const int*, const MatrixType*, const int*, RealType*),
	int m, int n, const MatrixType* Hrec, const MatrixType* Hinit, int ldh, RealType eps)
{
	RealType norm = 0;
	MatrixType* Hdiff = alloc_arr<MatrixType>(m * n);

	// Norm of residual
	for (int j = 0; j < n; j++)
#pragma omp simd
		for (int i = 0; i < m; i++)
			Hdiff[i + ldh * j] = Hrec[i + ldh * j] - Hinit[i + ldh * j];

	norm = LANGE("1", &m, &n, Hdiff, &ldh, NULL);
	norm = norm / LANGE("1", &m, &n, Hinit, &ldh, NULL);

	free_arr(Hdiff);

	return norm;
}

template <typename MatrixType>
double RelError2(double(*LANGE)(const char *, const int*, const int*, const MatrixType*, const int*, double *),
	int m, int n, const MatrixType *Hrec, int ldh1, const MatrixType *Hinit, int ldh2, double eps)
{
	double norm = 0;
	MatrixType *Hdiff = alloc_arr<MatrixType>(m * n); int ldh = m;

	// Norm of residual
#pragma omp parallel for schedule(static)
	for (int j = 0; j < n; j++)
#pragma omp simd
		for (int i = 0; i < m; i++)
			Hdiff[i + ldh * j] = Hrec[i + ldh1 * j] - Hinit[i + ldh2 * j];

	norm = LANGE("Frob", &m, &n, Hdiff, &ldh, NULL);
	norm = norm / LANGE("Frob", &m, &n, Hinit, &ldh2, NULL);

	free_arr(Hdiff);

	return norm;

	//if (norm < eps) printf("Norm %12.10e < eps %12.10lf: PASSED\n", norm, eps);
	//else printf("Norm %12.10lf > eps %12.10lf : FAILED\n", norm, eps);
}

template <typename MatrixType>
double AbsError(double(*LANGE)(const char *, const int*, const int*, const MatrixType*, const int*, double *),
	int m, int n, const MatrixType *Hrec, const MatrixType *Hinit, int ldh, double eps)
{
	double norm = 0;
	MatrixType *Hdiff = alloc_arr<MatrixType>(m * n);

	// Norm of residual
#pragma omp parallel for schedule(static)
	for (int j = 0; j < n; j++)
#pragma omp simd
		for (int i = 0; i < m; i++)
			Hdiff[i + ldh * j] = Hrec[i + ldh * j] - Hinit[i + ldh * j];

	norm = LANGE("Frob", &m, &n, Hdiff, &ldh, NULL);

	free_arr(Hdiff);

	return norm;
}

template <typename MatrixType>
void construct_block_row(void (*LACPY)(const char *, const int*, const int*, const MatrixType*, const int*, MatrixType*, const int*),
	int m, int n, MatrixType* BL, int ldbl, MatrixType *A, int lda, MatrixType *BR, int ldbr, MatrixType* Arow, int ldar)
{
	if (BL == NULL)
	{
		LACPY("All", &m, &n, A, &lda, &Arow[0 + ldar * 0], &ldar);
		LACPY("All", &m, &n, BR, &ldbr, &Arow[0 + ldar * n], &ldar);
	}
	else if (BR == NULL)
	{
		LACPY("All", &m, &n, BL, &ldbl, &Arow[0 + ldar * 0], &ldar);
		LACPY("All", &m, &n, A, &lda, &Arow[0 + ldar * n], &ldar);
	}
	else
	{
		LACPY("All", &m, &n, BL, &ldbl, &Arow[0 + ldar * 0], &ldar);
		LACPY("All", &m, &n, A, &lda, &Arow[0 + ldar * n], &ldar);
		LACPY("All", &m, &n, BR, &ldbr, &Arow[0 + ldar * 2 * n], &ldar);
	}
}

template<class t1, class t2>
void TestEqual(const t1& v1, const t2& v2, const string& hint = {})
{
	if (v1 != v2)
	{
		ostringstream os;
		os << "Assertion failed: " << v1 << " != " << v2 << ".";
		if (!hint.empty()) {
			os << " Hint: " << hint;
		}
	}
}

template<typename MatrixType>
void DeleteBoundaries(int n1, int n2, int n3, long int size1, MatrixType *a, long int size2, MatrixType *b)
{
	int i = 0, j = 0, k = 0;
	int numb = 0;

	if (size1 == size2)
	{
		printf("There is no boundaries reduction\n");

#pragma omp parallel for schedule(static)
		for (int i = 0; i < size1; i++)
			b[i] = a[i];

		return;
	}

	for (int l = 0; l < size1; l++)
	{
		take_coord3D(n1, n2, n3, l, i, j, k);
		if (i > 0 && j > 0 && k > 0 && i < (n1 - 1) && j < (n2 - 1) && k < (n3 - 1))
			b[numb++] = a[l];
	}

#ifdef PRINT
	if (numb != size2) printf("ERROR of reducing boundaries 3D: %d != %d\n", numb, size2);
	else printf("Boundaries 3D are reduced successfully!\n");
#endif
}


