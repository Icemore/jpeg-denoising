#include "ConjugateGradient.h"


double const eps = 1e-5;

double innerProduct(vector const & a, vector const & b)
{
	double res = 0;

	for(size_t i = 0; i < a.size(); ++i)
		res += a[i] * b[i];

	return res;
}

void mvProduct(matrix const & m, vector const &a, vector & res)
{
	for(size_t i = 0; i < m.size(); ++i)
	{
		res[i] = 0;
		for(size_t j = 0; j < m[i].size(); ++j)
			res[i] += m[i][j] * a[j];
	}
}

bool converged(vector const & r)
{
	for(size_t i = 0; i < r.size(); ++i)
		if(abs(r[i]) > eps)
			return false;

	return true;
}

void solveConjugateGradient(matrix& a, vector& b, vector& x)
{
	size_t const n = x.size();

	vector r[2];
	vector p(n), ap(n);
	int cur = 0;
	int next = 1;

	// r
	r[0].resize(n);
	r[1].resize(n);
	mvProduct(a, x, r[cur]);

	for(size_t i = 0; i < n; ++i)
		r[cur][i] -= b[i];

	// p
	for(size_t i = 0; i < n; ++i)
		p[i] = -r[cur][i];

	int k = 0;

	while(!converged(r[cur]) && k < 100000)
	{
		mvProduct(a, p, ap);

		double alpha = innerProduct(r[cur], r[cur]) / innerProduct(p, ap);

		// update x
		for(size_t i = 0; i < n; ++i)
			x[i] += alpha * p[i];

		//get r_{k+1}
		for(size_t i = 0; i < n; ++i)
			r[next][i] = r[cur][i] + alpha * ap[i];

		double beta = innerProduct(r[next], r[next]) / innerProduct(r[cur], r[cur]);

		// update p
		for(size_t i = 0; i < n; ++i)
			p[i] = -r[next][i] + beta * p[i];

		cur = next;
		next = 1 - cur;
		++k;
	}
}