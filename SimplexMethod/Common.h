#pragma once

#undef EPSILON
#define EPSILON 0.00001f

struct Fraction {
	int numerator;
	int denominator;
};

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

	void Resize(int NewRowNumber, int NewColNumber) {
		assert(NewRowNumber >= 0 && NewColNumber >= 0);
		Fraction* temp = matrix;
		matrix = new Fraction[NewRowNumber * NewColNumber];
		if (!matrix) { matrix = temp; }

		RowNumber = NewRowNumber;
		ColNumber = NewColNumber;
	}

	Fraction* operator[](int row) {
		assert(row >= 0 && row < RowNumber);
		return &matrix[row * ColNumber];
	}
};

struct Matrix {
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
		if (!matrix) { matrix = temp; }

		RowNumber = NewRowNumber;
		ColNumber = NewColNumber;
	}

	float* operator[](int row) {
		assert(row >= 0 && row < RowNumber);
		return &matrix[row * ColNumber];
	}

	Matrix& operator=(const Matrix &mat) {
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
	CONTINUE
};

struct Step {
	int StepID;
	RowAndColumn StepChosenRC;
	RowAndColumn LeadElementRC;
	bool IsAutomatic;
	bool IsWaitingForInput;
	bool IsCompleted;
	std::vector<int> RowsBannedToSwap;
	Matrix RealMatrix;
	FractionalMatrix FracMatrix;
	// First m variables are basis variables
	std::vector<int> NumbersOfVariables;
	bool IsArtificialStep;
	bool IsSolutionUmlimited;

	Step(Matrix RealMatrix) : StepID(0), RealMatrix(RealMatrix) { }

	Step(const Step& step) {
		StepID = step.StepID;
		StepChosenRC.Row = step.StepChosenRC.Row;
		StepChosenRC.Column = step.StepChosenRC.Column;
		LeadElementRC.Row = step.LeadElementRC.Row;
		LeadElementRC.Column = step.LeadElementRC.Column;
		IsAutomatic = step.IsAutomatic;
		IsWaitingForInput = step.IsWaitingForInput;
		IsCompleted = step.IsCompleted;
		RowsBannedToSwap = step.RowsBannedToSwap;
		RealMatrix = step.RealMatrix;
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
		RowsBannedToSwap = step.RowsBannedToSwap;
		RealMatrix = step.RealMatrix;
		NumbersOfVariables = step.NumbersOfVariables;
		IsArtificialStep = step.IsArtificialStep;
		return *this;
	}
};