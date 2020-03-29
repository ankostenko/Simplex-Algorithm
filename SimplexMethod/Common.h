#pragma once

#undef EPSILON
#define EPSILON 0.00001f

struct Fraction {
	int numerator;
	int denominator;
};

struct FractionalMatrix {
	Fraction* matrix;
	int RowNumber;
	int ColNumber;

	FractionalMatrix(int RowNumber, int ColNumber) : RowNumber(RowNumber), ColNumber(ColNumber) {
		assert(RowNumber >= 0 && ColNumber >= 0);
		matrix = new Fraction[RowNumber * ColNumber];
		assert(matrix);
	}

	~FractionalMatrix() {
		delete matrix;
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