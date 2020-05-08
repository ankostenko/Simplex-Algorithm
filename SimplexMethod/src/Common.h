#pragma once

#undef EPSILON
#define EPSILON 0.00001f

#define IS_SAME_TYPE(Type1, Type2) (std::is_same<Type1, Type2>::value)
#define DoesContain(Vector, Element) ((std::find((Vector).begin(), (Vector).end(), (Element))) != (Vector).end())

// Basic math functions
// --------------------
int gcd(int a, int b) {
	if (b == 0)
		return a;
	return gcd(b, a % b);
}

static int Clamp(int value, int min, int max) {
	if (value > max) return max;
	if (value < min) return min;

	return value;
}
//--------------------

struct Fraction {
	int numerator;
	int denominator;

	Fraction() = default;
	Fraction(int Num, int Denom) : numerator(Num), denominator(Denom) {}

	void NormalizeFraction() {
		if (denominator < 0) { denominator = -denominator; numerator = -numerator; }

		int CurGCD = abs(gcd(numerator, denominator));
		numerator /= CurGCD;
		denominator /= CurGCD;
	}

	Fraction operator*(float value) {
		int TempNumerator = numerator * value;
		Fraction Result = Fraction(TempNumerator, denominator);
		Result.NormalizeFraction();
		return Result;
	}

	Fraction operator*(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;
		TempNumerator *= other.numerator;
		TempDenominator *= other.denominator;
		Fraction result = Fraction(TempNumerator, TempDenominator);
		result.NormalizeFraction();
		return result;
	}

	bool operator>(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;

		if (denominator != other.denominator) {
			TempNumerator *= other.denominator;
			int TempOtherNumerator = other.numerator * denominator;
			// Overflow handling
			if (other.numerator > INT32_MAX / denominator) {
				float ApproxValue = (float)numerator / denominator;
				float ApproxOtherValue = (float)other.numerator / other.denominator;

				return ApproxValue > ApproxOtherValue;
			}
			other.numerator *= denominator;
		}

		return (TempNumerator > other.numerator);
	}

	bool operator>(int value) {
		value *= denominator;
		return numerator > value;
	}

	bool operator<(int value) {
		value *= denominator;
		return numerator < value;
	}

	bool operator<=(Fraction other) {
		long long LLNumerator = numerator;
		long long LLOtherNumerator = other.numerator;

		if (denominator != other.denominator) {
			LLNumerator *= other.denominator;
			LLOtherNumerator *= denominator;
		}

		return LLNumerator <= LLOtherNumerator;
	}

	bool operator>=(Fraction other) {
		long long LLNumerator = numerator;
		long long LLOtherNumerator = other.numerator;

		if (denominator != other.denominator) {
			LLNumerator *= other.denominator;
			LLOtherNumerator *= denominator;
		}

		return LLNumerator >= LLOtherNumerator;
	}

	bool operator==(Fraction other) {
		// Making sure values that are equal to zero reliably compared. It is purely a safety check
		if (numerator == 0 && other.numerator == 0) {
			return true;
		}

		int CurGCD = gcd(other.numerator, other.denominator);
		other.numerator /= CurGCD;
		other.denominator /= CurGCD;

		CurGCD = gcd(numerator, denominator);
		numerator /= CurGCD;
		denominator /= CurGCD;

		if (denominator != other.denominator) {
			int temp = denominator;
			denominator *= other.denominator;
			numerator *= other.denominator;
			other.numerator *= temp;
		}

		return ((numerator == other.numerator) && (denominator == other.denominator));
	}

	bool operator!=(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;

		if (denominator != other.denominator) {
			TempNumerator *= other.denominator;
			other.numerator *= denominator;
		}

		return (TempNumerator != other.numerator);
	}

	bool operator<(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;

		if (denominator != other.denominator) {
			TempNumerator *= other.denominator;
			int TempOtherNumerator = other.numerator * denominator;
			// Overflow handling
			if (other.numerator > INT32_MAX / denominator) {
				float ApproxValue = (float)numerator / denominator;
				float ApproxOtherValue = (float)other.numerator / other.denominator;

				return ApproxValue < ApproxOtherValue;
			}
			other.numerator *= denominator;
		}

		return (TempNumerator < other.numerator);
	}

	Fraction operator/(Fraction other) const {
		if (other.numerator == 0) {
			return Fraction(0, 1);
		}
		int TempNumerator = numerator;
		int TempDenominator = denominator;
		TempNumerator *= other.denominator;
		TempDenominator *= other.numerator;
		Fraction result = Fraction(TempNumerator, TempDenominator);
		result.NormalizeFraction();
		return result;
	}

	Fraction operator+(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;
		if (denominator != other.denominator) {
			int temp = denominator;
			TempDenominator *= other.denominator;
			TempNumerator *= other.denominator;
			other.numerator *= temp;
		}
		TempNumerator += other.numerator;

		Fraction result = Fraction(TempNumerator, TempDenominator);
		result.NormalizeFraction();
		return result;
	}

	Fraction operator-(Fraction other) const {
		int TempNumerator = numerator;
		int TempDenominator = denominator;
		if (denominator != other.denominator) {
			int temp = denominator;
			TempDenominator *= other.denominator;
			TempNumerator *= other.denominator;
			other.numerator *= temp;
		}
		TempNumerator -= other.numerator;
		Fraction result = Fraction(TempNumerator, TempDenominator);
		result.NormalizeFraction();
		return result;
	}

	Fraction operator-() const {
		return Fraction(-numerator, denominator);
	}

	Fraction& operator+=(const Fraction& other) {
		int OtherNumerator = other.numerator;
		if (denominator != other.denominator) {
			int temp = denominator;
			denominator *= other.denominator;
			numerator *= other.denominator;
			OtherNumerator *= temp;
		}

		numerator += OtherNumerator;
		NormalizeFraction();
		return *(this);
	}

	Fraction& operator=(const Fraction& other) {
		numerator = other.numerator;
		denominator = other.denominator;
		return *this;
	}
};

template<typename Type> Type Genfabs(Type value) {
	if constexpr (IS_SAME_TYPE(Type, float)) {
		return fabs(value);
	} else {
		return Fraction(fabs(value.numerator), value.denominator);
	}
}

struct FractionalMatrix {
	Fraction* matrix = NULL;
	int RowNumber;
	int ColNumber;

	FractionalMatrix() = default;

	FractionalMatrix(int RowNumber, int ColNumber) : RowNumber(RowNumber), ColNumber(ColNumber) {
		assert(RowNumber >= 0 && ColNumber >= 0);
		matrix = new Fraction[RowNumber * ColNumber];
		assert(matrix);
	}

	~FractionalMatrix() {
		delete[] matrix;
	}

	FractionalMatrix(const FractionalMatrix& mat) {
		matrix = new Fraction[mat.RowNumber * mat.ColNumber];
		memmove(matrix, mat.matrix, mat.RowNumber * mat.ColNumber * sizeof(Fraction));
		RowNumber = mat.RowNumber;
		ColNumber = mat.ColNumber;
	}

	void DeleteColumn(int NumberOfColumn) {
		assert(NumberOfColumn >= 0 && NumberOfColumn < ColNumber);

		Fraction* temp = new Fraction[RowNumber * (ColNumber - 1)];
		for (int i = 0; i < RowNumber; i++) {
			for (int j = 0, jNew = 0; j < ColNumber; j++, jNew++) {
				if (j == NumberOfColumn) {
					jNew -= 1;
					continue;
				}

				temp[i * (ColNumber - 1) + jNew] = (*this)[i][j];
			}
		}

		delete[] matrix;
		matrix = temp;

		ColNumber -= 1;
	}

	void Resize(int NewRowNumber, int NewColNumber) {
		assert(NewRowNumber >= 0 && NewColNumber >= 0);
		Fraction* temp = matrix;
		matrix = new Fraction[NewRowNumber * NewColNumber];
		if (!matrix) {
			matrix = temp;
		} else {
			delete[] temp;
		}

		RowNumber = NewRowNumber;
		ColNumber = NewColNumber;
	}

	Fraction* operator[](int row) {
		assert(row >= 0 && row < RowNumber);
		return &matrix[row * ColNumber];
	}

	FractionalMatrix& operator=(const FractionalMatrix& mat) {
		delete[] matrix;
		matrix = new Fraction[mat.RowNumber * mat.ColNumber];
		memmove(matrix, mat.matrix, mat.RowNumber * mat.ColNumber * sizeof(Fraction));
		RowNumber = mat.RowNumber;
		ColNumber = mat.ColNumber;
		return *this;
	}

	void SwapColumns(int IdxC1, int IdxC2) {
		assert(IdxC1 >= 0 && IdxC1 < ColNumber - 1);
		assert(IdxC2 >= 0 && IdxC2 < ColNumber - 1);
		for (int i = 0; i < RowNumber; i++) {
			std::swap((*this)[i][IdxC1], (*this)[i][IdxC2]);
		}
	}

	void SwapRows(int IdxR1, int IdxR2) {
		assert(IdxR1 >= 0 && IdxR1 < RowNumber - 1);
		assert(IdxR2 >= 0 && IdxR2 < RowNumber - 1);
		for (int Col = 0; Col < ColNumber; Col++) {
			std::swap((*this)[IdxR1][Col], (*this)[IdxR2][Col]);
		}
	}
};

class Matrix {
public:
	float* matrix = NULL;
	int RowNumber;
	int ColNumber;

	Matrix() = default;

	Matrix(int RowNumber, int ColNumber) : RowNumber(RowNumber), ColNumber(ColNumber) {
		assert(RowNumber >= 0 && ColNumber >= 0);
		matrix = new float[RowNumber * ColNumber];
		assert(matrix);
	}

	~Matrix() {
		delete[] matrix;
	}

	Matrix(const Matrix& mat) {
		matrix = new float[mat.RowNumber * mat.ColNumber];
		memmove(matrix, mat.matrix, mat.RowNumber * mat.ColNumber * sizeof(float));
		RowNumber = mat.RowNumber;
		ColNumber = mat.ColNumber;
	}

	// TODO: It is pretty bad solution but for now it's okay
	void DeleteColumn(int NumberOfColumn) {
		assert(NumberOfColumn >= 0 && NumberOfColumn < ColNumber);

		float* temp = new float[RowNumber * (ColNumber - 1)];
		for (int i = 0; i < RowNumber; i++) {
			for (int j = 0, jNew = 0; j < ColNumber; j++, jNew++) {
				if (j == NumberOfColumn) { 
					jNew -= 1;
					continue; 
				}

				temp[i * (ColNumber - 1) + jNew] = (*this)[i][j];
			}
		}

		delete[] matrix;
		matrix = temp;

		ColNumber -= 1;
	}

	void Resize(int NewRowNumber, int NewColNumber) {
		assert(NewRowNumber >= 0 && NewColNumber >= 0);
		float* temp = matrix;
		matrix = new float[NewRowNumber * NewColNumber];
		if (!matrix) {  
			matrix = temp; 
		} else { 
			delete[] temp;
		}

		RowNumber = NewRowNumber;
		ColNumber = NewColNumber;
	}

	float* operator[](int row) {
		assert(row >= 0 && row < RowNumber);
		return &matrix[row * ColNumber];
	}

	Matrix& operator=(const Matrix &mat) {
		delete[] matrix;
		matrix = new float[mat.RowNumber * mat.ColNumber];
		memmove(matrix, mat.matrix, mat.RowNumber * mat.ColNumber * sizeof(float));
		RowNumber = mat.RowNumber;
		ColNumber = mat.ColNumber;
		return *this;
	}

	void SwapColumns(int IdxC1, int IdxC2) {
		assert(IdxC1 >= 0 && IdxC1 < ColNumber - 1);
		assert(IdxC2 >= 0 && IdxC2 < ColNumber - 1);
		for (int i = 0; i < RowNumber; i++) {
			std::swap((*this)[i][IdxC1], (*this)[i][IdxC2]);
		}
	}

	void SwapRows(int IdxR1, int IdxR2) {
		assert(IdxR1 >= 0 && IdxR1 < RowNumber - 1);
		assert(IdxR2 >= 0 && IdxR2 < RowNumber - 1);
		for (int Col = 0; Col < ColNumber; Col++) {
			std::swap((*this)[IdxR1][Col], (*this)[IdxR2][Col]);
		}
	}
};

struct RowAndColumn {
	int Row;
	int Column;
};

enum AlgorithmState {
	UNDEFINED,
	COMPLETED,
	UNLIMITED_SOLUTION,
	CONTINUE,
	SOLUTION_DOESNT_EXIST,
};

struct Step {
	int StepID;
	RowAndColumn StepChosenRC;
	RowAndColumn LeadElementRC;
	bool IsAutomatic;
	bool IsWaitingForInput;
	bool IsCompleted;
	Matrix RealMatrix;
	FractionalMatrix FracMatrix;
	// First m variables are basis variables
	std::vector<int> NumbersOfVariables;
	bool IsArtificialStep;
	bool IsSolutionUmlimited;

	Step() = default;

	Step(Matrix RealMatrix, FractionalMatrix FracMatrix) : StepID(0), RealMatrix(RealMatrix), FracMatrix(FracMatrix) { }

	Step(const Step& step) {
		StepID = step.StepID;
		StepChosenRC.Row = step.StepChosenRC.Row;
		StepChosenRC.Column = step.StepChosenRC.Column;
		LeadElementRC.Row = step.LeadElementRC.Row;
		LeadElementRC.Column = step.LeadElementRC.Column;
		IsAutomatic = step.IsAutomatic;
		IsWaitingForInput = step.IsWaitingForInput;
		IsCompleted = step.IsCompleted;
		RealMatrix = step.RealMatrix;
		FracMatrix = step.FracMatrix;
		NumbersOfVariables = step.NumbersOfVariables;
		IsArtificialStep = step.IsArtificialStep;
	}

	Step &operator=(const Step& step) {
		StepID = step.StepID;
		StepChosenRC.Row = step.StepChosenRC.Row;
		StepChosenRC.Column = step.StepChosenRC.Column;
		LeadElementRC.Row =    step.LeadElementRC.Row;
		LeadElementRC.Column = step.LeadElementRC.Column;
		IsAutomatic = step.IsAutomatic;
		IsWaitingForInput = step.IsWaitingForInput;
		IsCompleted = step.IsCompleted;
		RealMatrix = step.RealMatrix;
		FracMatrix = step.FracMatrix;
		NumbersOfVariables = step.NumbersOfVariables;
		IsArtificialStep = step.IsArtificialStep;
		return *this;
	}
};