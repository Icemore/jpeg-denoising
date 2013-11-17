#include <vector>

typedef std::vector< std::vector<double> > matrix;
typedef std::vector<double> vector;

void solveConjugateGradient(matrix& a, vector& b, vector& x);
