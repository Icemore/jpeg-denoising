#define _CRT_SECURE_NO_DEPRECATE
#include <iostream>

#include "ConjugateGradient.h"

void print(vector const & x)
{
	for(size_t i = 0; i < x.size(); ++i)
	{
		std::cout << x[i] <<", ";
	}

	std::cout << std::endl;
}


/* 
	f(x, y) = x^2 + y^2
	x >= 2
	y <= 3
*/
void solveParabola(vector & x, vector & lambda)
{
	double eps = 1e-10;
	double mu = 1;

	const size_t n = x.size();
	matrix a;
	vector b(n);

	a.assign(n, vector(n));

	a[0][1]=0;
	a[1][0]=0;

	for(size_t k = 0; k  < 10000; ++k )
	{
		if(x[0]-2-mu*lambda[0] < eps)
		{
			a[0][0] = 2+1/mu;
			b[0] = lambda[0] + 2/mu;
		}
		else
		{
			a[0][0]=2;
			b[0]=0;
		}

		if(-x[1] + 3 -mu*lambda[1] < eps)
		{
			a[1][1] = 2+1/mu;
			b[1] = -lambda[1] + 3/mu;
		}
		else
		{
			a[1][1] = 2;
			b[1] = 0;
		}

		solveConjugateGradient(a, b, x);

		lambda[0] = std::max(lambda[0] - (x[0] -2) /mu, 0.0);
		lambda[1] = std::max(lambda[1] - (-x[1]+3)/mu, 0.0);
	}
}


int main()
{
	freopen("input.txt", "r", stdin);

	int n;
	std::cin>>n;
	vector x(n), lambda(n);


	for(size_t i = 0; i < n; ++i)
		std::cin >> lambda[i];
	

	for(size_t i = 0; i < n; ++i)
		std::cin >> x[i];

	
	solveParabola(x, lambda);

	print(x);

	return 0;
}