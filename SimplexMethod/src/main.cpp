/*
		[] Работа с обыкновенными и десятичными дробями.
		[] Контроль данных (защита от «дурака»)
		[] [Сохранение введённой задачи в файл] и чтение из файла.
		[] В пошаговом режиме возможность возврата назад.
		[] Справка.
		[] Контекстно-зависимая помощь.
		[x] Возможность решения задачи с использованием заданных базисных переменных.
		[x] Возможность диалогового ввода размерности задачи и матрицы коэффициентов целевой функции в канонической форме. Размерность не более 16*16.
		[x] Реализация метода искусственного базиса.
		[x] Выбор автоматического и пошагового режима решения задачи.
		[x] В пошаговом режиме возможность выбора опорного элемента.
		[x] Поддержка мыши.
*/

// BUG: Program doesn't show anything if there's no any negative elements in columns and result is negative. (Should show doesn't have solutions(?))


#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <stdio.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <type_traits>
#include <algorithm>

#include "portable-file-dialogs.h"
#include "Common.h"
#include "GUILayer.h"

std::vector<Step> ArtificialBasisSteps;
std::vector<Step> SimplexAlgorithmSteps;
std::vector<Step> ExplicitBasisSteps;

AlgorithmState CheckAlgorithmState(Matrix& matrix, bool IsAutomatic, bool IsArtificialStep) {
	AlgorithmState state = UNDEFINED;

	for (int i = 0; i < matrix.ColNumber - 1; i++) {
		if (matrix[matrix.RowNumber - 1][i] < -EPSILON) {
			state = UNDEFINED;
			// Check if there is at least one positive element in a column
			for (int j = 0; j < matrix.RowNumber - 1; j++) {
				if (matrix[j][i] > EPSILON) {
					state = CONTINUE;
					break;
				}
			}
			if (state == UNDEFINED) {
				state = UNLIMITED_SOLUTION;
				break;
			}
		}
	}

	if (state == UNDEFINED) {
		state = COMPLETED;

		// Check if system of equalities have solutions
		if (IsArtificialStep) {
			if (matrix[matrix.RowNumber - 1][matrix.ColNumber - 1] < -EPSILON) {
				state = SOLUTION_DOESNT_EXIST;
			}
		}
	}

	assert(state != UNDEFINED);
	return state;
}

AlgorithmState CheckAlgorithmState(FractionalMatrix& matrix, bool IsAutomatic, bool IsArtificialStep) {
	AlgorithmState state = UNDEFINED;

	for (int i = 0; i < matrix.ColNumber - 1; i++) {
		// Last row contains negative numbers
		if (matrix[matrix.RowNumber - 1][i] < 0) {
			state = UNDEFINED;
			// Check if there is at least one positive element in a column
			for (int j = 0; j < matrix.RowNumber - 1; j++) {
				if (matrix[j][i] > 0) {
					state = CONTINUE;
					break;
				}
			}
			if (state == UNDEFINED) {
				state = UNLIMITED_SOLUTION;
				break;
			}
		}
	}

	if (state == UNDEFINED) {
		state = COMPLETED;

		// Check if system of equalities have solutions
		if (IsArtificialStep) {
			if (matrix[matrix.RowNumber - 1][matrix.ColNumber - 1] < -EPSILON) {
				state = SOLUTION_DOESNT_EXIST;
			}
		}
	}

	assert(state != UNDEFINED);
	return state;
}

template<typename MatrixType, typename ElementType> Step SimplexStep(Step step) {
	int CurrentColumnIndex = -1;
	int CurrentRowIndex = -1;
	ElementType CurrentLead;

	if (step.IsCompleted) {
		return step;
	}

	// Choose matrix based on input
	MatrixType matrix;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		// Real case
		matrix = step.RealMatrix;
	} else {
		// Fractional case
		matrix = step.FracMatrix;
	}

	// Define generic zero for both real and fractional cases
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(ElementType, float)) {
		// Real case
		ZeroElement = EPSILON;
	} else {
		// Fractional case
		ZeroElement = Fraction(0, 1);
	}

	// Check what algorithm state is
	AlgorithmState state = UNDEFINED;
	state = CheckAlgorithmState(matrix, step.IsAutomatic, step.IsArtificialStep);
	assert(state != UNDEFINED);

	if (state == UNLIMITED_SOLUTION) {
		step.IsCompleted = true;
		return step;
	} else if (state == COMPLETED) {
		step.IsCompleted = true;
		return step;
	} else if (state == SOLUTION_DOESNT_EXIST) {
		step.IsCompleted = true;
		return step;
	}

	// Choose lead element
	if (step.IsAutomatic) {
		CurrentRowIndex = -1;
		// Assigning appropriate values to these variables
		ElementType ColumnMinimum;
		if constexpr (IS_SAME_TYPE(ElementType, float)) {
			// Real case
			CurrentLead = FLT_MAX;
			ColumnMinimum = FLT_MAX;
		} else {
			// Fractional case
			CurrentLead = Fraction(INT32_MAX, 1);
			ColumnMinimum = Fraction(INT32_MAX, 1);
		}

		// Find number of column of an available element
		for (int i = 0; i < matrix.RowNumber - 1; i++) {
			if (step.IsArtificialStep) {
				bool IsRowBannedToSwap = false;
				for (auto RowNumber : step.RowsBannedToSwap) {
					if (RowNumber == i) {
						IsRowBannedToSwap = true;
					}
				}
				//if (IsRowBannedToSwap) { continue; }
			}

			for (int j = 0; j < matrix.ColNumber - 1; j++) {
				if (matrix[matrix.RowNumber - 1][j] < -ZeroElement) {
					if (matrix[i][j] > ZeroElement) {
						CurrentColumnIndex = j;
						break;
					}
				}
			}
		}

		// Choose any available lead element
		for (int i = 0; i < matrix.RowNumber - 1; i++) {
			if (step.IsArtificialStep) {
				bool IsRowBannedToSwap = false;
				for (auto RowNumber : step.RowsBannedToSwap) {
					if (RowNumber == i) {
						IsRowBannedToSwap = true;
					}
				}
				//if (IsRowBannedToSwap) { continue; }
			}

			if (matrix[i][CurrentColumnIndex] > ZeroElement) {
				if (matrix[i][matrix.ColNumber - 1] / matrix[i][CurrentColumnIndex] < ColumnMinimum) {
					CurrentLead = matrix[i][CurrentColumnIndex];
					ColumnMinimum = matrix[i][matrix.ColNumber - 1] / matrix[i][CurrentColumnIndex];
					CurrentRowIndex = i;
				}
			}
		}
	} else {
		CurrentColumnIndex = step.LeadElementRC.Column;
		CurrentRowIndex = step.LeadElementRC.Row;

		// Assignment of lead element
		CurrentLead = matrix[step.LeadElementRC.Row][step.LeadElementRC.Column];
	}

	assert(CurrentRowIndex != -1);
	assert(CurrentColumnIndex != -1);
	if constexpr (IS_SAME_TYPE(MatrixType, FractionalMatrix)) {
		assert(CurrentLead.numerator != -1);
	} else {
		assert(CurrentLead != -1);
	}

	// Lead element is equal to 1 / Lead
	if constexpr (IS_SAME_TYPE(ElementType, float)) {
		// Real case
		matrix[CurrentRowIndex][CurrentColumnIndex] = 1 / CurrentLead;
	} else {
		// Fractional case
		matrix[CurrentRowIndex][CurrentColumnIndex] = Fraction(1, 1) / CurrentLead;
	}

	// Divide Row by lead
	for (int i = 0; i < matrix.ColNumber; i++) {
		if (i == CurrentColumnIndex) { continue; }
		matrix[CurrentRowIndex][i] = matrix[CurrentRowIndex][i] / CurrentLead;
	}

	// Divide Column by negative lead
	for (int i = 0; i < matrix.RowNumber; i++) {
		if (i == CurrentRowIndex) { continue; }
		matrix[i][CurrentColumnIndex] = (matrix[i][CurrentColumnIndex] / (-CurrentLead));
	}

	// Subtract all other rows by lead row
	for (int i = 0; i < matrix.RowNumber; i++) {
		if (i == CurrentRowIndex) { continue; }
		for (int j = 0; j < matrix.ColNumber; j++) {
			if (j == CurrentColumnIndex) { continue; }
			matrix[i][j] = matrix[i][j] - CurrentLead * matrix[i][CurrentColumnIndex] * matrix[CurrentRowIndex][j] * (-1);
		}
	}

	// Assign new step matricies
	Step NewStep = step;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		// Real case
		NewStep.RealMatrix = matrix;
		NewStep.FracMatrix = step.FracMatrix;
	} else {
		// Fractional case
		NewStep.RealMatrix = step.RealMatrix;
		NewStep.FracMatrix = matrix;
	}
	NewStep.StepID += 1;
	NewStep.StepChosenRC.Row = CurrentRowIndex;
	NewStep.StepChosenRC.Column = CurrentColumnIndex;
	NewStep.RowsBannedToSwap.push_back(CurrentRowIndex);
	return NewStep;
}

void MakeArtificialFunctionCoefficients(Matrix& matrix) {
	for (int i = 0; i < matrix.ColNumber; i++) {
		float ColumnSum = 0.0f;
		for (int j = 0; j < matrix.RowNumber - 1; j++) {
			ColumnSum += matrix[j][i];
		}
		matrix[matrix.RowNumber - 1][i] = -ColumnSum;
	}
}

void MakeArtificialFunctionCeofficients(FractionalMatrix& matrix) {
	for (int i = 0; i < matrix.ColNumber; i++) {
		Fraction ColumnSum = Fraction(0, 1);
		for (int j = 0; j < matrix.RowNumber - 1; j++) {
			ColumnSum = ColumnSum + matrix[j][i];
		}
		matrix[matrix.RowNumber - 1][i] = -ColumnSum;
	}
}

void MakeSimplexAlgorithmFunctionCoefficients(Step& step, std::vector<float>& RealTargetFunction) {
	Matrix TargetFunctionCoefficientsMatrix(step.RealMatrix);

	// Copy table one to one multiplying basis rows on coefficients of target function
	for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
		for (int j = 0; j < step.RealMatrix.ColNumber; j++) {
			if (j < step.RealMatrix.ColNumber - 1) {
				TargetFunctionCoefficientsMatrix[i][j] = (-1) * RealTargetFunction[step.NumbersOfVariables[i] - 1] * step.RealMatrix[i][j];
			} else {
				TargetFunctionCoefficientsMatrix[i][j] = RealTargetFunction[step.NumbersOfVariables[i] - 1] * step.RealMatrix[i][j];
			}
		}
	}

	// Fill new target function coefficients
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.ColNumber - 1; i++) {
		float ColumnSum = 0.0f;
		for (int j = 0; j < TargetFunctionCoefficientsMatrix.RowNumber - 1; j++) {
			ColumnSum += TargetFunctionCoefficientsMatrix[j][i];
		}
		ColumnSum += RealTargetFunction[step.NumbersOfVariables[i + step.RealMatrix.RowNumber - 1] - 1];
		step.RealMatrix[step.RealMatrix.RowNumber - 1][i] = ColumnSum;
	}

	// Free coefficient
	float ColumnSum = 0.0f;
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.RowNumber - 1; i++) {
		ColumnSum += TargetFunctionCoefficientsMatrix[i][TargetFunctionCoefficientsMatrix.ColNumber - 1];
	}
	ColumnSum += RealTargetFunction[RealTargetFunction.size() - 1];
	step.RealMatrix[step.RealMatrix.RowNumber - 1][step.RealMatrix.ColNumber - 1] = -ColumnSum;
}

template<typename MatrixType, typename ElementType> void MakeSimplexAlgorithmFunctionCoefficients(MatrixType& matrix, std::vector<int> NumbersOfVariables, std::vector<ElementType>& TargetFunction) {
	MatrixType TargetFunctionCoefficientsMatrix(matrix);

	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}

	// Copy table one to one multiplying basis rows on coefficients of target function
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		for (int j = 0; j < matrix.ColNumber; j++) {
			if (j < matrix.ColNumber - 1) {
				TargetFunctionCoefficientsMatrix[i][j] = TargetFunction[NumbersOfVariables[i] - 1] * matrix[i][j] * (-1);
			} else {
				TargetFunctionCoefficientsMatrix[i][j] = TargetFunction[NumbersOfVariables[i] - 1] * matrix[i][j];
			}
		}
	}

	// Fill new target function coefficients
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.ColNumber - 1; i++) {
		ElementType ColumnSum;
		if constexpr (IS_SAME_TYPE(ElementType, float)) {
			ColumnSum = 0.0f;
		} else {
			ColumnSum = Fraction(0, 1);
		}
		for (int j = 0; j < TargetFunctionCoefficientsMatrix.RowNumber - 1; j++) {
			ColumnSum = ColumnSum + TargetFunctionCoefficientsMatrix[j][i];
		}
		ColumnSum = ColumnSum + TargetFunction[NumbersOfVariables[i + matrix.RowNumber - 1] - 1];
		matrix[matrix.RowNumber - 1][i] = ColumnSum;
	}

	// Free coefficient
	ElementType ColumnSum = ZeroElement;
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.RowNumber - 1; i++) {
		ColumnSum = ColumnSum + TargetFunctionCoefficientsMatrix[i][TargetFunctionCoefficientsMatrix.ColNumber - 1];
	}
	ColumnSum = ColumnSum + TargetFunction[TargetFunction.size() - 1];
	matrix[matrix.RowNumber - 1][matrix.ColNumber - 1] = -ColumnSum;
}

void MakeSimplexAlgorithmFunctionCoefficients(Step& step, std::vector<Fraction>& FracTargetFunction) {
	FractionalMatrix TargetFunctionCoefficientsMatrix(step.FracMatrix);

	// Copy table one to one multiplying basis rows on coefficients of target function
	for (int i = 0; i < step.FracMatrix.RowNumber - 1; i++) {
		for (int j = 0; j < step.FracMatrix.ColNumber; j++) {
			if (j < step.FracMatrix.ColNumber - 1) {
				TargetFunctionCoefficientsMatrix[i][j] = FracTargetFunction[step.NumbersOfVariables[i] - 1] * step.FracMatrix[i][j] * (-1);
			} else {
				TargetFunctionCoefficientsMatrix[i][j] = FracTargetFunction[step.NumbersOfVariables[i] - 1] * step.FracMatrix[i][j];
			}
		}
	}

	// Fill new target function coefficients
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.ColNumber - 1; i++) {
		Fraction ColumnSum = Fraction(0, 1);
		for (int j = 0; j < TargetFunctionCoefficientsMatrix.RowNumber - 1; j++) {
			ColumnSum += TargetFunctionCoefficientsMatrix[j][i];
		}
		ColumnSum += FracTargetFunction[step.NumbersOfVariables[i + step.FracMatrix.RowNumber - 1] - 1];
		step.FracMatrix[step.FracMatrix.RowNumber - 1][i] = ColumnSum;
	}

	// Free coefficient
	Fraction ColumnSum = Fraction(0, 1);
	for (int i = 0; i < TargetFunctionCoefficientsMatrix.RowNumber - 1; i++) {
		ColumnSum += TargetFunctionCoefficientsMatrix[i][TargetFunctionCoefficientsMatrix.ColNumber - 1];
	}
	ColumnSum += FracTargetFunction[FracTargetFunction.size() - 1];
	step.FracMatrix[step.FracMatrix.RowNumber - 1][step.FracMatrix.ColNumber - 1] = -ColumnSum;
}

static int PreviousArtificialStepID = -1;
template<typename MatrixType, typename ElementType> void ArtificialBasis(Step step) {
	// Clear all leads each new iteration
	GUILayer::PotentialLeads.clear();

	// Depending on type of matrix choose one of those to use
	MatrixType matrix;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		matrix = step.RealMatrix;
	} else {
		matrix = step.FracMatrix;
	}

	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}


	if (step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	AlgorithmState state = UNDEFINED;
	state = CheckAlgorithmState(matrix, step.IsAutomatic, step.IsArtificialStep);

	if (state == UNLIMITED_SOLUTION) {
		step.IsCompleted = true;
	} else if (state == COMPLETED) {
		step.IsCompleted = true;
	} else if (state == SOLUTION_DOESNT_EXIST) {
		step.IsCompleted = true;
		ArtificialBasisSteps.push_back(step);
	}

	// Choose lead element
	if (!step.IsAutomatic) {
		ImGui::PushID("Choose Lead Element");
		int CurrentRowIndex = -1;
		static int Column = 0;
		static int CurrentCellIndex = 0;

		// Pair of id and column number
		std::vector<RowAndColumn> Leads;
		std::vector<std::pair<ElementType, int>> MinimumAndRowIndex;
		for (int i = 0; i < matrix.ColNumber - 1; i++) {
			CurrentRowIndex = -1;
			// Assign Fraction of float type
			ElementType CurrentLead;
			ElementType ColumnMinimum;
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				CurrentLead = FLT_MAX;
				ColumnMinimum = FLT_MAX;
			} else {
				CurrentLead = Fraction(INT32_MAX, 1);
				ColumnMinimum = Fraction(INT32_MAX, 1);
			}

			MinimumAndRowIndex.clear();

			// Check for negative element
			if (matrix[matrix.RowNumber - 1][i] < -ZeroElement) {
				for (int j = 0; j < matrix.RowNumber - 1; j++) {
					if (matrix[j][i] > ZeroElement) {
						ElementType TEMPDEBUG = matrix[j][matrix.ColNumber - 1] / matrix[j][i];
						// We add any elements that are equal to current minimum
						if (matrix[j][matrix.ColNumber - 1] / matrix[j][i] <= (ColumnMinimum + ZeroElement)) {
							ColumnMinimum = matrix[j][matrix.ColNumber - 1] / matrix[j][i];
							CurrentLead = matrix[j][i];
							CurrentRowIndex = j;
							MinimumAndRowIndex.push_back(std::make_pair(ColumnMinimum, CurrentRowIndex));
						}
					}
				}
			}

			if (CurrentRowIndex != -1) {
				// Delete all elements that are not minimum
				// There can be more than one minimum
				// I keep all the minimums
				for (int i = 0; i < MinimumAndRowIndex.size(); i++) {
					auto MR = MinimumAndRowIndex[i];
					// Check if a value not equal to minimum
					bool IsCurrentNotEqualToMinimum;
					if constexpr (IS_SAME_TYPE(ElementType, float)) {
						IsCurrentNotEqualToMinimum = (fabs(MR.first - ColumnMinimum)) > EPSILON;
					} else {
						IsCurrentNotEqualToMinimum = (MR.first != ColumnMinimum);
					}

					if (IsCurrentNotEqualToMinimum) {
						MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
						i = -1;
					}
				}

				for (std::pair<ElementType, int> MR : MinimumAndRowIndex) {
					bool IsRowBanned = false;
					for (auto row : step.RowsBannedToSwap) {
						if (row == MR.second) {
							IsRowBanned = true;
						}
					}

					// Multiple Minimums
					//if (!IsRowBanned) {
					GUILayer::PotentialLeads.push_back(RowAndColumn({ MR.second, i }));
					//}
				}
			}
		}

		if (!step.IsCompleted && GUILayer::PotentialLeads.size() != 0) {
			// Choose first available leading element
			if (PreviousArtificialStepID != step.StepID) {
				GUILayer::CurrentLeadPos.Column = GUILayer::PotentialLeads[0].Column;
				GUILayer::CurrentLeadPos.Row = GUILayer::PotentialLeads[0].Row;
				PreviousArtificialStepID = step.StepID;
			}

			if (ImGui::Button(u8"Подтвердить")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = GUILayer::CurrentLeadPos.Column;
				step.LeadElementRC.Row = GUILayer::CurrentLeadPos.Row;
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	} else {
		step.IsWaitingForInput = false;
	}

	if (!step.IsWaitingForInput) {
		int RowNumber = matrix.RowNumber;
		for (int iteration = 0; iteration < RowNumber; iteration++) {
			// Calculate current step
			Step NewStep = SimplexStep<MatrixType, ElementType>(step);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(RowNumber - 1) + NewStep.StepChosenRC.Column]);
			for (int i = RowNumber - 1; i < NewStep.NumbersOfVariables.size(); i++) {
				if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
					if (NewStep.NumbersOfVariables[i] > ArtificialBasisSteps[1].RealMatrix.ColNumber - 1) {
						NewStep.NumbersOfVariables.erase(NewStep.NumbersOfVariables.begin() + i);
						NewStep.RealMatrix.DeleteColumn(i - (RowNumber - 1));
						break;
					}
				} else {
					if (NewStep.NumbersOfVariables[i] > NewStep.FracMatrix.ColNumber - 1) {
						NewStep.NumbersOfVariables.erase(NewStep.NumbersOfVariables.begin() + (RowNumber - 1) + i);
						NewStep.FracMatrix.DeleteColumn(i);
						i -= 2;
					}
				}
			}

			// Disables "confirm" button
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				// Real case
				AlgorithmState state = CheckAlgorithmState(NewStep.RealMatrix, false, NewStep.IsArtificialStep);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			} else {
				// Fractional case
				AlgorithmState state = CheckAlgorithmState(NewStep.FracMatrix, false, NewStep.IsArtificialStep);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			}

			if (!NewStep.IsAutomatic) {
				NewStep.IsWaitingForInput = true;
				ArtificialBasisSteps.push_back(NewStep);
				break;
			}
			ArtificialBasisSteps.push_back(NewStep);
			step = NewStep;
		}
	}
}

static int PreviousSimplexStepID = -1;
template<typename MatrixType, typename ElementType> void SimplexAlgorithm(Step step) {
	GUILayer::PotentialLeads.clear();

	if (!step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	// Depending on type of matrix choose one of those to use
	MatrixType matrix;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		matrix = step.RealMatrix;
	} else {
		matrix = step.FracMatrix;
	}

	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}

	AlgorithmState state = UNDEFINED;
	state = CheckAlgorithmState(matrix, step.IsAutomatic, step.IsArtificialStep);
	assert(state != UNDEFINED);

	if (state == UNLIMITED_SOLUTION) {
		step.IsCompleted = true;
	} else if (state == COMPLETED) {
		step.IsCompleted = true;
	}

	// Choose lead element
	if (!step.IsAutomatic) {
		ImGui::PushID("Choose Lead Element");
		int CurrentRowIndex = -1;
		static int Column = 0;
		static int CurrentCellIndex = 0;

		// Pair of id and column number
		std::vector<RowAndColumn> Leads;
		std::vector<std::pair<ElementType, int>> MinimumAndRowIndex;
		for (int i = 0; i < matrix.ColNumber - 1; i++) {
			CurrentRowIndex = -1;
			// Assign Fraction of float type
			ElementType CurrentLead;
			ElementType ColumnMinimum;
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				CurrentLead = FLT_MAX;
				ColumnMinimum = FLT_MAX;
			} else {
				CurrentLead = Fraction(INT32_MAX, 1);
				ColumnMinimum = Fraction(INT32_MAX, 1);
			}

			MinimumAndRowIndex.clear();

			// Check for negative element
			if (matrix[matrix.RowNumber - 1][i] < -ZeroElement) {
				for (int j = 0; j < matrix.RowNumber - 1; j++) {
					if (matrix[j][i] > ZeroElement) {
						ElementType TEMPDEBUG = matrix[j][matrix.ColNumber - 1] / matrix[j][i];
						// We add any elements that are equal to current minimum
						if (matrix[j][matrix.ColNumber - 1] / matrix[j][i] <= (ColumnMinimum + ZeroElement)) {
							ColumnMinimum = matrix[j][matrix.ColNumber - 1] / matrix[j][i];
							CurrentLead = matrix[j][i];
							CurrentRowIndex = j;
							MinimumAndRowIndex.push_back(std::make_pair(ColumnMinimum, CurrentRowIndex));
						}
					}
				}
			}

			if (state == CONTINUE && CurrentRowIndex != -1) {
				// Delete all elements that are not minimum
				// There can be more than one minimum
				// I keep all the minimums
				for (int i = 0; i < MinimumAndRowIndex.size(); i++) {
					auto MR = MinimumAndRowIndex[i];
					// Check if a value not equal to minimum
					bool IsCurrentNotEqualToMinimum;
					if constexpr (IS_SAME_TYPE(ElementType, float)) {
						IsCurrentNotEqualToMinimum = (fabs(MR.first - ColumnMinimum)) > EPSILON;
					} else {
						IsCurrentNotEqualToMinimum = (MR.first != ColumnMinimum);
					}

					if (IsCurrentNotEqualToMinimum) {
						MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
						i = -1;
					}
				}

				for (std::pair<ElementType, int> MR : MinimumAndRowIndex) {
					bool IsRowBanned = false;
					for (auto row : step.RowsBannedToSwap) {
						if (row == MR.second) {
							IsRowBanned = true;
						}
					}
					GUILayer::PotentialLeads.push_back(RowAndColumn({ MR.second, i }));

				}
			}
		}

		if (!step.IsCompleted) {
			// Choose first available leading element
			if (PreviousSimplexStepID != step.StepID && GUILayer::PotentialLeads.size() != 0) {
				GUILayer::CurrentLeadPos.Column = GUILayer::PotentialLeads[0].Column;
				GUILayer::CurrentLeadPos.Row = GUILayer::PotentialLeads[0].Row;
				PreviousSimplexStepID = step.StepID;
			}

			if (ImGui::Button(u8"Подтвердить")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = GUILayer::CurrentLeadPos.Column;
				step.LeadElementRC.Row = GUILayer::CurrentLeadPos.Row;
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	} else {
		step.IsWaitingForInput = false;
	}

	if (!step.IsWaitingForInput) {
		int RowNumber = matrix.RowNumber;
		for (int iteration = 0; iteration < RowNumber; iteration++) {
			Step NewStep = SimplexStep<MatrixType, ElementType>(step);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(RowNumber - 1) + NewStep.StepChosenRC.Column]);

			// Disables "confirm" button
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				// Real case
				AlgorithmState state = CheckAlgorithmState(NewStep.RealMatrix, false, NewStep.IsArtificialStep);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			} else {
				// Fractional case
				AlgorithmState state = CheckAlgorithmState(NewStep.FracMatrix, false, NewStep.IsArtificialStep);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			}

			// If it is not automatic execution rise waiting for input flag
			if (!NewStep.IsAutomatic) {
				NewStep.IsWaitingForInput = true;
				SimplexAlgorithmSteps.push_back(NewStep);
				break;
			}
			SimplexAlgorithmSteps.push_back(NewStep);
			step = NewStep;
		}
	}
}

template<typename MatrixType, typename ElementType> void GaussElimination(MatrixType& matrix) {
	int PivotRow = 0;
	int PivotColumn = 0;
	int num = 2;

	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}

	while (PivotRow < (matrix.RowNumber - 1) && PivotColumn < matrix.ColNumber - 1) {
		// Find max value in a column
		ElementType MaxPivot;
		MaxPivot = Genfabs(matrix[PivotRow][PivotColumn]);

		int MaxPivotIdx = PivotRow;
		for (int i = PivotRow; i < matrix.RowNumber - PivotRow - 1 - 1; i++) {
			if (Genfabs(matrix[i][PivotColumn]) > MaxPivot) {
				MaxPivot = Genfabs(matrix[i][PivotColumn]);
				MaxPivotIdx = i;
			}
		}

		if (Genfabs(matrix[MaxPivotIdx][PivotColumn]) <= ZeroElement) {
			PivotColumn += 1;
		} else {
			matrix.SwapRows(PivotRow, MaxPivotIdx);

			// Substract all rows below pivot
			for (int i = PivotRow + 1; i < matrix.RowNumber - 1; i++) {

				ElementType Factor = matrix[i][PivotColumn] / matrix[PivotRow][PivotColumn];
				if constexpr (IS_SAME_TYPE(ElementType, float)) {
					matrix[i][PivotColumn] = 0.0f;
				} else {
					matrix[i][PivotColumn] = Fraction(0, 1);
				}

				for (int j = PivotColumn + 1; j < matrix.ColNumber; j++) {
					matrix[i][j] = matrix[i][j] - matrix[PivotRow][j] * Factor;
				}
			}

			PivotRow++;
			PivotColumn++;
		}
	}

	// Backward substitution
	for (int i = matrix.RowNumber - 2; i > 0; i--) {
		if (Genfabs(matrix[i][i]) <= ZeroElement) continue;
		for (int j = i; j > 0; j--) {
			ElementType Factor = matrix[j - 1][i] / matrix[i][i];
			for (int k = i; k < matrix.ColNumber - 1; k++) {
				matrix[j - 1][k] = matrix[j - 1][k] - matrix[i][k] * Factor;
			}

			matrix[j - 1][matrix.ColNumber - 1] = matrix[j - 1][matrix.ColNumber - 1] - matrix[i][matrix.ColNumber - 1] * Factor;
		}
	}

	// Normalization
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		ElementType Pivot = matrix[i][i];
		if (Genfabs(Pivot) <= ZeroElement) continue;
		for (int j = i; j < matrix.ColNumber; j++) {
			matrix[i][j] = matrix[i][j] / Pivot;
		}
	}
}

void Print(Matrix& matrix) {
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		for (int j = 0; j < matrix.ColNumber; j++) {
			printf("%f ", matrix[i][j]);
		}
		printf("\n");
	}
}

template<typename MatrixType, typename ElementType>Step ExplicitBasis(Step step, MatrixType& matrix, std::vector<ElementType>& ExplicitBasis, std::vector<ElementType>& TargetFunction) {
	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}

	// 1. Prepare matrix to Gauss elimination
	std::vector<int> PositionsOfNonZeroElements;
	std::vector<int> VariablesPositions;
	for (int i = 0; i < ExplicitBasis.size(); i++) {
		VariablesPositions.push_back(i + 1);
	}

	// 1.1 Find positions of non-zero elements
	for (int i = 0; i < ExplicitBasis.size(); i++) {
		if (Genfabs(ExplicitBasis[i]) >= ZeroElement) {
			PositionsOfNonZeroElements.push_back(i);
		}
	}

	// 1.2 Swap matrix column and variables to first (RowNumber) columns
	for (int i = 0; i < PositionsOfNonZeroElements.size(); i++) {
		int NonZeroPosition = PositionsOfNonZeroElements[i];
		matrix.SwapColumns(i, NonZeroPosition);
		std::swap(VariablesPositions[i], VariablesPositions[NonZeroPosition]);
	}

	// 2. Gauss Elimination
	GaussElimination<MatrixType, ElementType>(matrix);

	// 3. Complete table for first step of simplex alogrithm
	// 3.1 Delete first RowNumber columns
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		matrix.DeleteColumn(0);
	}

	step.NumbersOfVariables = VariablesPositions;
	MakeSimplexAlgorithmFunctionCoefficients(matrix, step.NumbersOfVariables, TargetFunction);
	return step;
}

int main() {
	// Problem characteristics
	int NumberOfVariables;
	int NumberOfLimitations;
	int IsAutomatic = 1;
	int SizeConfirmedClicked = 0;
	int OldSizeCondfirmedClicked = 0;
	
	int IsArtificialBasis = true;
	int IsFractionalCoefficients = 0;
	
	int UnconfirmedIsArtificialBasis = true;
	int UnconfirmedIsFractionalCoefficients = 0;
	int UnconfirmedIsAutomatic = 1;
	
	bool ShowSolution = false;
	bool StartSimplexAlgorithm = false;
	bool SizeConfirmedReadyToContinue = false;
	bool ContinueToProblemInput = false;
	std::vector<Fraction> FractionalTargetFunction(1);
	std::vector<float> RealTargetFunction(1);
	std::vector<Fraction> FractionalExplicitBasis(1);
	std::vector<float> RealExplicitBasis(1);
	Matrix RealMatrix(1, 1);
	FractionalMatrix FracMatrix(1, 1);
	Step step(RealMatrix, FracMatrix);
	ArtificialBasisSteps.push_back(step);

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 768, u8"Симплекс Алгоритм", 0, 0);
	assert(window);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize OpenGL context");
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.Fonts->AddFontFromFileTTF("OpenSans-Regular.ttf", 20, NULL, io.Fonts->GetGlyphRangesCyrillic());

	ImGui::StyleColorsLight();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430 core");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Main menu bar
		// --------------------------------------------
		static bool OpenAboutPopup = false;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu(u8"Файл")) {
				if (ImGui::MenuItem(u8"Открыть...", "CTRL+O")) {
					printf("Open menu clicked\n");
				}
				if (ImGui::MenuItem(u8"Сохранить", "CTRL+S")) {
					printf("Save menu clicked\n");
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(u8"Справка")) {
				if (ImGui::MenuItem(u8"О программе")) {
					OpenAboutPopup = true;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
		if (OpenAboutPopup) { ImGui::OpenPopup(u8"О программе"); }

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x * 0.7f, 160.0f));
		if (ImGui::BeginPopupModal(u8"О программе", 0, ImGuiWindowFlags_NoResize)) {
			ImGui::BeginChild("About", ImVec2(0, 0), true);
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::GetFontSize() * 3.8f);
			ImGui::Text(u8"Симплекс алгоритм");
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::GetFontSize() * 5);
			ImGui::Text(u8"Костенко Андрей, ИВТ-31БО");
			ImGui::NewLine();
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - 30.0f);
			if (ImGui::Button("OK", ImVec2(60.0f, 25.0f))) {
				OpenAboutPopup = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndChild();
			ImGui::EndPopup();
		}
		// --------------------------------------------

		// Docking 
		// --------------------------------------------
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_AutoHideTabBar;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen) {
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->GetWorkPos());
			ImGui::SetNextWindowSize(viewport->GetWorkSize());
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
			window_flags |= ImGuiWindowFlags_NoBackground;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", 0, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen) {
			ImGui::PopStyleVar(2);
		}

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		ImGui::End();
		// -------------------------------------------

		// Configuration Window
		ImGui::Begin(u8"Конфигурация задачи", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

		// Size title
		ImGui::Text(u8"Выберите размерность задачи");
		ImGui::Separator();

		// Choose number of variables
		NumberOfVariables = GUILayer::ComboBox(GUILayer::ComboBoxVariablesLabel);
		// Choose number of limitations
		NumberOfLimitations = GUILayer::ComboBox(GUILayer::ComboBoxLimitationsLabel);

		// Apply new sizes
		if (ImGui::Button(u8"Применить")) {
			// +1 for function coefficients			   // +1 for vector B
			RealMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables + 1);
			RealTargetFunction.resize(NumberOfVariables + 1);
			RealExplicitBasis.resize(NumberOfVariables);

			FracMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables + 1);
			FractionalTargetFunction.resize(NumberOfVariables + 1);
			FractionalExplicitBasis.resize(NumberOfVariables);

			SizeConfirmedReadyToContinue = true;
		}
		ImGui::SameLine(); GUILayer::HelpMarker(u8"Размерность будет изменена только после нажатия кнопки 'Применить'.");
		ImGui::Separator();

		// Size is confirmed we can proceed further
		if (SizeConfirmedReadyToContinue) {
			// Choose between real number and fractions
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
			ImGui::Combo(u8"Тип элементов", &UnconfirmedIsFractionalCoefficients, u8"Действительные числа\0Обыкновенные дроби\0");
			ImGui::Separator();

			// Choose between explicit and artificial basis
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
			ImGui::Combo(u8"Тип базиса", &UnconfirmedIsArtificialBasis, u8"Явный базис\0Искусственный базис\0");
			ImGui::Separator();

			// Choose between step by step and automatic solution modes
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
			ImGui::Combo(u8"Режим решения", &UnconfirmedIsAutomatic, u8"Пошаговый\0Автоматический\0");
			ImGui::Separator();

			// Apply all properties and continue
			ImGui::PushID("Properties Apply");
			if (ImGui::Button(u8"Применить")) {
				ContinueToProblemInput = true;
				Step FirstStep = ArtificialBasisSteps[0];
				ArtificialBasisSteps.clear();
				ArtificialBasisSteps.push_back(FirstStep);
				SimplexAlgorithmSteps.clear();
				ExplicitBasisSteps.clear();
				ShowSolution = false;
				StartSimplexAlgorithm = false;
				PreviousSimplexStepID = -1;
				PreviousArtificialStepID = -1;

				IsFractionalCoefficients = UnconfirmedIsFractionalCoefficients;
				IsArtificialBasis = UnconfirmedIsArtificialBasis;
				IsAutomatic = UnconfirmedIsAutomatic;
			}
			ImGui::SameLine(); GUILayer::HelpMarker(u8"Параметры решения изменяются только после нажатия кнопки 'Применить'.");
			ImGui::PopID();

			ImGui::Separator();
		}
		ImGui::End();

		// Data input window
		ImGui::Begin(u8"Ввод данных", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
		if (ContinueToProblemInput) {
			// Input of explicit basis
			if (!IsArtificialBasis) {
				ImGui::Text(u8"Введите базис");

				ImGui::PushID("Explicit Basis");
				if (!IsFractionalCoefficients) {
					ImGui::PushID("Real Basis");
					// Real basis input
					GUILayer::InputVector(RealExplicitBasis, RealExplicitBasis.size(),
						// Print Function
						[](int Size) {
							for (int i = 0; i < Size; i++) {
								ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
								ImGui::NextColumn();
							}
						});
					ImGui::PopID();
				} else {
					ImGui::PushID("Fractional Basis");
					// Fractional basis input
					GUILayer::InputVector(FractionalExplicitBasis, FractionalExplicitBasis.size(),
						// Print Function
						[](int Size) {
							for (int i = 0; i < Size; i++) {
								ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
								ImGui::NextColumn();
							}
							ImGui::NextColumn();
						});
					ImGui::PopID();
				}
				ImGui::PopID();
				ImGui::Separator();
			}

			// Matrix of limitations
			ImGui::PushID("Matrix");
			ImGui::Text(u8"Матрица ограничений");
			ImGui::SameLine(); GUILayer::HelpMarker(u8"*Внутри этого блока переключаться между элементами можно с помощью клавиши Tab.\n*Изменить ширину столбца можно потянув за вертикальный разделитель.");
			// Group
			ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + RealMatrix.ColNumber * 140, 0.0f));
			ImGui::BeginChild("Matrix Of Limitations", ImVec2(0, ImGui::GetFontSize() * RealMatrix.RowNumber * 2), false, ImGuiWindowFlags_HorizontalScrollbar);

			if (!IsFractionalCoefficients) {
				GUILayer::MatrixInput(RealMatrix);
			} else {
				GUILayer::MatrixInput(FracMatrix);
			}
			ImGui::EndChild();
			ImGui::PopID();


			ImGui::Separator();

			// Target function coefficient input
			ImGui::Text(u8"Целевая функция");
			if (!IsFractionalCoefficients) {
				ImGui::PushID("Real Function");
				// Target function input with lambda expression
				GUILayer::InputVector(RealTargetFunction, RealTargetFunction.size(),
					// Print Function
					[](int Size) {
						for (int i = 0; i < Size - 1; i++) {
							ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
							ImGui::NextColumn();
						}
						ImGui::Text("C");
						ImGui::NextColumn();
					});
				ImGui::PopID();
			} else {
				ImGui::PushID("Fractional Function");

				// Target function input with lambda expression
				GUILayer::InputVector(FractionalTargetFunction, FractionalTargetFunction.size(),
					// Print Function
					[](int Size) {
						for (int i = 0; i < Size - 1; i++) {
							ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
							ImGui::NextColumn();
						}
						ImGui::Text("C");
						ImGui::NextColumn();
					});

				ImGui::PopID();
			}

			ImGui::Spacing();

			if (ImGui::Button(u8"Решить")) {
				ShowSolution = true;

				Step FirstStep = ArtificialBasisSteps[0];
				ArtificialBasisSteps.clear();
				ArtificialBasisSteps.push_back(FirstStep);
				SimplexAlgorithmSteps.clear();
				ExplicitBasisSteps.clear();
				StartSimplexAlgorithm = false;
				PreviousSimplexStepID = -1;
				PreviousArtificialStepID = -1;
			}
			GUILayer::HelpMarker(u8"'Решить' при повторном нажатии начинает решение сначала."); ImGui::SameLine();

			if (ShowSolution) {
				ImGui::SameLine();
				if (ImGui::Button(u8"Отменить решение")) {
					Step FirstStep = ArtificialBasisSteps[0];
					ArtificialBasisSteps.clear();
					ArtificialBasisSteps.push_back(FirstStep);
					SimplexAlgorithmSteps.clear();
					ExplicitBasisSteps.clear();
					ShowSolution = false;
					StartSimplexAlgorithm = false;
					PreviousSimplexStepID = -1;
					PreviousArtificialStepID = -1;
				}
			}
		}
		ImGui::End();
		// ====================================
		//		VERIFIED UNTIL THAT POINT
		// ====================================

		// Jump here if window was closed
		BeforeShowSolutionTarget:
		if (ShowSolution) {
			ImGui::Begin(u8"Решение", &ShowSolution, ImGuiWindowFlags_AlwaysVerticalScrollbar);
			// If solution window was closed we need to restore state to non-solved
			if (ShowSolution == false) {
				Step FirstStep = ArtificialBasisSteps[0];
				ArtificialBasisSteps.clear();
				ArtificialBasisSteps.push_back(FirstStep);
				SimplexAlgorithmSteps.clear();
				ExplicitBasisSteps.clear();
				StartSimplexAlgorithm = false;
				PreviousSimplexStepID = -1;
				PreviousArtificialStepID = -1;
				ImGui::End();
				goto BeforeShowSolutionTarget;
			}

			ImGui::BeginTabBar("Solutions");

			// SimplexAlgorithmTabFlags
			ImGuiTabItemFlags SimplexAlgorithmTabFlags = ImGuiTabItemFlags_None;

			// Artificial Basis Step
			if (IsArtificialBasis) {
				if (ImGui::BeginTabItem(u8"Искусственный базис")) {
					// Calculate next step of simplex algorithm
					size_t LastElementIndex = ArtificialBasisSteps.size() - 1;
					step = ArtificialBasisSteps[LastElementIndex];
					if (ArtificialBasisSteps.size() == 1) {
						step.RealMatrix = RealMatrix;
						step.FracMatrix = FracMatrix;
						step.IsAutomatic = IsAutomatic;
						step.IsCompleted = false;
						step.IsArtificialStep = true;
						// Artificial variables
						if (IsFractionalCoefficients) {
							for (int i = 0; i < step.FracMatrix.RowNumber - 1; i++) {
								step.NumbersOfVariables.push_back(step.FracMatrix.ColNumber + i);
							}
						} else {
							for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
								step.NumbersOfVariables.push_back(step.RealMatrix.ColNumber + i);
							}
						}
						// Variables
						if (IsFractionalCoefficients) {
							for (int i = 0; i < step.FracMatrix.ColNumber - 1; i++) {
								step.NumbersOfVariables.push_back(i + 1);
							}
						} else {
							for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
								step.NumbersOfVariables.push_back(i + 1);
							}
						}
						if (IsFractionalCoefficients) {
							MakeArtificialFunctionCeofficients(step.FracMatrix);
						} else {
							MakeArtificialFunctionCoefficients(step.RealMatrix);
						}
						ArtificialBasisSteps.push_back(step);
					}


					if (!step.IsAutomatic) {
						step.IsWaitingForInput = true;
					} else {
						step.IsWaitingForInput = false;
					}

					// Display all steps that has been calculated
					ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + RealMatrix.ColNumber * 170, 0.0f));
					ImGui::BeginChild("Matrix Of Limitations", ImVec2(ImGui::GetWindowWidth() - ImGui::GetCursorPos().x - 25.0f, ImGui::GetWindowHeight() * 0.7f), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
					GUILayer::DisplaySteps(ArtificialBasisSteps, 1, IsFractionalCoefficients);
					if (!IsFractionalCoefficients) {
						ArtificialBasis<Matrix, float>(step);
					} else {
						ArtificialBasis<FractionalMatrix, Fraction>(step);
					}
					ImGui::Separator();
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
			} else {
				// Explicit basis tab
				if (ImGui::BeginTabItem(u8"Решение для заданного базиса")) {
					// Explicit Basis Steps
					if (ExplicitBasisSteps.size() == 0) {
						step.StepID = 0;
						step.RealMatrix = RealMatrix;
						step.FracMatrix = FracMatrix;
						step.IsAutomatic = IsAutomatic;
						step.IsCompleted = false;
						step.IsArtificialStep = false;
						// TODO: Add input checks
						// [] Check if input vector doesn't lead to degenerate matrix
						// [] If number of non-zero elements in the vector less then number of limitations I need to randomly choose any available variable
						//		or drop this method and use artificial basis instead
						// First check if number of non-zero elements in the explicit vector less or equal then a number of limitations
						int AmountOfNonZeroElements = 0;
						for (float el : RealExplicitBasis) {
							if (fabs(el) > EPSILON) {
								AmountOfNonZeroElements += 1;
							}
						}

						if (AmountOfNonZeroElements > step.RealMatrix.RowNumber - 1) {
							printf("\nNumber of non-zero elements greater then number of limitations\n");
							assert(0);
						}

						if (IsFractionalCoefficients) {
							// Fractional case
							FractionalMatrix matrix = step.FracMatrix;
							step = ExplicitBasis(step, matrix, FractionalExplicitBasis, FractionalTargetFunction);
							step.FracMatrix = matrix;
						} else {
							// Real case
							Matrix matrix = step.RealMatrix;
							step = ExplicitBasis(step, matrix, RealExplicitBasis, RealTargetFunction);
							step.RealMatrix = matrix;
						}
						step.IsCompleted = true;

						ExplicitBasisSteps.push_back(step);
					}

					ImGui::PushID("Explicit Basis Displaying");
					ImGui::BeginChild("Solution", ImVec2(0, 100), true);
					ImGui::Text(u8"Ответ");
					ImGui::Separator();
					if (IsFractionalCoefficients) {
						// Fractional case
						GUILayer::DisplaySolutionVector(step.FracMatrix, step.NumbersOfVariables, false);
					} else {
						// Real case
						GUILayer::DisplaySolutionVector(step.RealMatrix, step.NumbersOfVariables, false);
					}
					ImGui::EndChild();

					if (ImGui::Button(u8"Продолжить симплекс алгоритм")) {
						StartSimplexAlgorithm = true;
						SimplexAlgorithmTabFlags |= ImGuiTabItemFlags_SetSelected;
					}
					ImGui::PopID();
					ImGui::EndTabItem();
				}
			}

			ImGui::Separator();

			// Sholution has been found
			if (step.IsCompleted && step.IsArtificialStep) {
				ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + RealMatrix.ColNumber * 170, 0.0f));
				ImGui::BeginChild("Solution", ImVec2(ImGui::GetWindowWidth() - ImGui::GetCursorPos().x - 25.0f, 130.0f), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
				ImGui::Text(u8"Ответ");
				ImGui::Separator();

				// Display Solution
				if (IsFractionalCoefficients) {
					AlgorithmState state = CheckAlgorithmState(step.FracMatrix, false, step.IsArtificialStep);
					if (state != CONTINUE) {

						assert(state != UNDEFINED);

						if (state == UNLIMITED_SOLUTION) {
							ImGui::TextColored(ImColor(255, 0, 0), u8"Решение неограничено!");
						} else if (state == SOLUTION_DOESNT_EXIST) {
							ImGui::TextColored(ImColor(255, 0, 0), u8"Решения не существует!");
						} else if (state == COMPLETED) {
							// Fractional case
							GUILayer::DisplaySolutionVector(step.FracMatrix, step.NumbersOfVariables, false);
						}
					}
				} else {
					AlgorithmState state = CheckAlgorithmState(step.RealMatrix, false, step.IsArtificialStep);
					if (state != CONTINUE) {
						assert(state != UNDEFINED);

						if (state == UNLIMITED_SOLUTION) {
							ImGui::TextColored(ImColor(255, 0, 0), u8"Решение неограничено!");
						} else if (state == SOLUTION_DOESNT_EXIST) {
							ImGui::TextColored(ImColor(255, 0, 0), u8"Решения не существует!");
						} else if (state == COMPLETED) {
							// Real case
							GUILayer::DisplaySolutionVector(step.RealMatrix, step.NumbersOfVariables, false);
						}
					}
				}

				ImGui::EndChild();

				if (!step.IsCompleted && ImGui::Button(u8"Продолжить симплекс алгоритм")) {
					StartSimplexAlgorithm = true;
					SimplexAlgorithmTabFlags |= ImGuiTabItemFlags_SetSelected;
				}
			}

			if (StartSimplexAlgorithm) {
				ImGui::SetNextWindowFocus();
				if (ImGui::BeginTabItem(u8"Симплекс алгоритм", &StartSimplexAlgorithm, SimplexAlgorithmTabFlags)) {
					// Simplex algorithm's tab has been closed
					if (StartSimplexAlgorithm == false) {
						SimplexAlgorithmSteps.clear();
					}

					if (SimplexAlgorithmSteps.size() == 0) {
						if (IsArtificialBasis) {
							size_t LastElementIndex = ArtificialBasisSteps.size() - 1;
							step.RealMatrix = ArtificialBasisSteps[LastElementIndex].RealMatrix;
							step.FracMatrix = ArtificialBasisSteps[LastElementIndex].FracMatrix;
							step.NumbersOfVariables = ArtificialBasisSteps[LastElementIndex].NumbersOfVariables;
							step.IsCompleted = false;
							if (IsFractionalCoefficients) {
								MakeSimplexAlgorithmFunctionCoefficients(step, FractionalTargetFunction);
							} else {
								MakeSimplexAlgorithmFunctionCoefficients(step, RealTargetFunction);
							}
						} else {
							size_t LastElementIndex = ExplicitBasisSteps.size() - 1;
							step.RealMatrix = ExplicitBasisSteps[LastElementIndex].RealMatrix;
							step.FracMatrix = ExplicitBasisSteps[LastElementIndex].FracMatrix;
							step.NumbersOfVariables = ExplicitBasisSteps[LastElementIndex].NumbersOfVariables;
							step.IsCompleted = false;
							if (IsFractionalCoefficients) {
								MakeSimplexAlgorithmFunctionCoefficients(step, FractionalTargetFunction);
							} else {
								MakeSimplexAlgorithmFunctionCoefficients(step, RealTargetFunction);
							}
						}

						step.StepID = 0;
						step.IsAutomatic = IsAutomatic;
						step.IsCompleted = false;
						step.IsArtificialStep = false;

						SimplexAlgorithmSteps.push_back(step);
					} else {
						size_t LastSimplexAlgorithmElementIndex = SimplexAlgorithmSteps.size() - 1;
						step = SimplexAlgorithmSteps[LastSimplexAlgorithmElementIndex];
					}
					ImGui::PushID("Simplex Algorithm");
					ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + RealMatrix.ColNumber * 170, 0.0f));
					ImGui::BeginChild("Matrix Of Limitations", ImVec2(ImGui::GetWindowWidth() - ImGui::GetCursorPos().x - 25.0f, ImGui::GetWindowHeight() * 0.7f), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
					GUILayer::DisplaySteps(SimplexAlgorithmSteps, 0, IsFractionalCoefficients);
					if (IsFractionalCoefficients) {
						SimplexAlgorithm<FractionalMatrix, Fraction>(step);
					} else {
						SimplexAlgorithm<Matrix, float>(step);
					}
					ImGui::EndChild();
					ImGui::PopID();

					// Display Solution
					if (IsFractionalCoefficients) {
						AlgorithmState state = CheckAlgorithmState(step.FracMatrix, false, step.IsArtificialStep);
						if (state != CONTINUE) {
							ImGui::BeginChild("Solution", ImVec2(0, 0), true);
							ImGui::Text(u8"Ответ");
							ImGui::Separator();

							assert(state != UNDEFINED);
							assert(state != SOLUTION_DOESNT_EXIST);

							if (state == UNLIMITED_SOLUTION) {
								ImGui::TextColored(ImColor(255, 0, 0), u8"Решение неограничено!");
							} else if (state == COMPLETED) {
								// Fractional case
								GUILayer::DisplaySolutionVector(step.FracMatrix, step.NumbersOfVariables, true);
							}
							ImGui::EndChild();
						}
					} else {
						AlgorithmState state = CheckAlgorithmState(step.RealMatrix, false, step.IsArtificialStep);
						if (state != CONTINUE) {
							ImGui::BeginChild("Solution", ImVec2(0, 0), true);
							ImGui::Text(u8"Ответ");
							ImGui::Separator();

							assert(state != UNDEFINED);
							assert(state != SOLUTION_DOESNT_EXIST);

							if (state == UNLIMITED_SOLUTION) {
								ImGui::TextColored(ImColor(255, 0, 0), u8"Решение неограничено!");
							} else if (state == COMPLETED) {
								// Real case
								GUILayer::DisplaySolutionVector(step.RealMatrix, step.NumbersOfVariables, true);
							}
							ImGui::EndChild();
						}
					}
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
			ImGui::End();
		}


		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return 0;
}