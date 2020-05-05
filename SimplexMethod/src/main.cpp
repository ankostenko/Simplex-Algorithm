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
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <stdio.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <type_traits>

#include "Common.h"
#include "GUILayer.h"

std::vector<Step> ArtificialBasisSteps;
std::vector<Step> SimplexAlgorithmSteps;
std::vector<Step> ExplicitBasisSteps;

void PrintMatrix(Matrix& matrix) {
	for (int i = 0; i < matrix.RowNumber; i++) {
		for (int j = 0; j < matrix.ColNumber; j++) {
			printf("%f ", matrix[i][j]);
		}
		printf("\n");
	}
}

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
		if (matrix[matrix.RowNumber - 1][matrix.ColNumber - 1] < -EPSILON) {
			state = SOLUTION_DOESNT_EXIST;
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
		if (matrix[matrix.RowNumber - 1][matrix.ColNumber - 1] < -EPSILON) {
			state = SOLUTION_DOESNT_EXIST;
		}
	}

	assert(state != UNDEFINED);
	return state;
}

Step SimplexStep(Step step, bool IsFractionalCoefficients) {
	int CurrentColumnIndex = -1;
	int CurrentRowIndex = -1;
	float RealCurrentLead = -1;
	Fraction FracCurrentLead = Fraction(-1, 1);

	AlgorithmState state = UNDEFINED;
	if (IsFractionalCoefficients) {
		state = CheckAlgorithmState(step.FracMatrix, step.IsAutomatic, step.IsArtificialStep);
	} else {
		state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep);
	}
	assert(state != UNDEFINED);

	if (state == UNLIMITED_SOLUTION) {
		printf("Solution is unlimited");
		step.IsCompleted = true;
		return step;
	} else if (state == COMPLETED) {
		printf("Algorithm is completed");
		step.IsCompleted = true;
		return step;
	} else if (state == SOLUTION_DOESNT_EXIST) {
		printf("Solution doesn't exist");
		step.IsCompleted = true;
		return step;
	}

	// Choose lead element
	if (step.IsAutomatic) {
		if (IsFractionalCoefficients) {
			CurrentRowIndex = -1;
			FracCurrentLead.numerator = INT32_MAX;
			Fraction ColumnMinimum = Fraction(INT32_MAX, 1);

			// Find number of column of an available element
			for (int i = 0; i < step.FracMatrix.RowNumber - 1; i++) {
				if (step.IsArtificialStep) {
					bool IsRowBannedToSwap = false;
					for (auto RowNumber : step.RowsBannedToSwap) {
						if (RowNumber == i) {
							IsRowBannedToSwap = true;
						}
					}
					if (IsRowBannedToSwap) { continue; }
				}

				for (int j = 0; j < step.FracMatrix.ColNumber - 1; j++) {
					if (step.FracMatrix[step.FracMatrix.RowNumber - 1][j] < 0) {
						if (step.FracMatrix[i][j] > 0) {
							CurrentColumnIndex = j;
							break;
						}
					}
				}
			}

			// Choose any available lead element
			for (int i = 0; i < step.FracMatrix.RowNumber - 1; i++) {
				if (step.FracMatrix[i][CurrentColumnIndex] > 0) {
					if (step.FracMatrix[i][step.FracMatrix.ColNumber - 1] / step.FracMatrix[i][CurrentColumnIndex] < ColumnMinimum) {
						FracCurrentLead = step.FracMatrix[i][CurrentColumnIndex];
						ColumnMinimum = step.FracMatrix[i][step.FracMatrix.ColNumber - 1] / step.FracMatrix[i][CurrentColumnIndex];
						CurrentRowIndex = i;
					}
				}
			}
		} else {
			CurrentRowIndex = -1;
			RealCurrentLead = FLT_MAX;
			float ColumnMinimum = FLT_MAX;

			// Find number of column of an available element
			for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
				if (step.IsArtificialStep) {
					bool IsRowBannedToSwap = false;
					for (auto RowNumber : step.RowsBannedToSwap) {
						if (RowNumber == i) {
							IsRowBannedToSwap = true;
						}
					}
					if (IsRowBannedToSwap) { continue; }
				}

				for (int j = 0; j < step.RealMatrix.ColNumber - 1; j++) {
					if (step.RealMatrix[step.RealMatrix.RowNumber - 1][j] < -EPSILON) {
						if (step.RealMatrix[i][j] > EPSILON) {
							CurrentColumnIndex = j;
							break;
						}
					}
				}
			}

			// Choose any available lead element
			for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
				if (step.RealMatrix[i][CurrentColumnIndex] > EPSILON) {
					if (step.RealMatrix[i][step.RealMatrix.ColNumber - 1] / step.RealMatrix[i][CurrentColumnIndex] < ColumnMinimum) {
						RealCurrentLead = step.RealMatrix[i][CurrentColumnIndex];
						ColumnMinimum = step.RealMatrix[i][step.RealMatrix.ColNumber - 1] / step.RealMatrix[i][CurrentColumnIndex];
						CurrentRowIndex = i;
					}
				}
			}
		}
	} else {
		CurrentColumnIndex = step.LeadElementRC.Column;
		CurrentRowIndex = step.LeadElementRC.Row;
		if (IsFractionalCoefficients) {
			FracCurrentLead = step.FracMatrix[step.LeadElementRC.Row][step.LeadElementRC.Column];
		} else {
			RealCurrentLead = step.RealMatrix[step.LeadElementRC.Row][step.LeadElementRC.Column];
		}
	}

	assert(CurrentRowIndex != -1);
	assert(CurrentColumnIndex != -1);
	if (IsFractionalCoefficients) {
		assert(FracCurrentLead.numerator != -1);
	} else {
		assert(RealCurrentLead != -1);
	}

	if (IsFractionalCoefficients) {
		// Lead element is equal to 1 / Lead
		step.FracMatrix[CurrentRowIndex][CurrentColumnIndex] = Fraction(1, 1) / FracCurrentLead;

		// Divide Row by lead
		for (int i = 0; i < step.FracMatrix.ColNumber; i++) {
			if (i == CurrentColumnIndex) { continue; }
			step.FracMatrix[CurrentRowIndex][i] = step.FracMatrix[CurrentRowIndex][i] / FracCurrentLead;
		}

		// Divide Column by negative lead
		for (int i = 0; i < step.FracMatrix.RowNumber; i++) {
			if (i == CurrentRowIndex) { continue; }
			step.FracMatrix[i][CurrentColumnIndex] = step.FracMatrix[i][CurrentColumnIndex] / (-FracCurrentLead);
		}

		// Subtract all other rows by lead row
		for (int i = 0; i < step.FracMatrix.RowNumber; i++) {
			if (i == CurrentRowIndex) { continue; }
			for (int j = 0; j < step.FracMatrix.ColNumber; j++) {
				if (j == CurrentColumnIndex) { continue; }
				step.FracMatrix[i][j] = step.FracMatrix[i][j] - FracCurrentLead * step.FracMatrix[i][CurrentColumnIndex] * step.FracMatrix[CurrentRowIndex][j] * (-1);
			}
		}
	} else {
		// Lead element is equal to 1 / Lead
		step.RealMatrix[CurrentRowIndex][CurrentColumnIndex] = 1 / RealCurrentLead;

		// Divide Row by lead
		for (int i = 0; i < step.RealMatrix.ColNumber; i++) {
			if (i == CurrentColumnIndex) { continue; }
			step.RealMatrix[CurrentRowIndex][i] /= RealCurrentLead;
		}

		// Divide Column by negative lead
		for (int i = 0; i < step.RealMatrix.RowNumber; i++) {
			if (i == CurrentRowIndex) { continue; }
			step.RealMatrix[i][CurrentColumnIndex] /= -RealCurrentLead;
		}

		// Subtract all other rows by lead row
		for (int i = 0; i < step.RealMatrix.RowNumber; i++) {
			if (i == CurrentRowIndex) { continue; }
			for (int j = 0; j < step.RealMatrix.ColNumber; j++) {
				if (j == CurrentColumnIndex) { continue; }
				step.RealMatrix[i][j] -= (-1) * RealCurrentLead * step.RealMatrix[i][CurrentColumnIndex] * step.RealMatrix[CurrentRowIndex][j];
			}
		}
	}

	Step NewStep(step.RealMatrix, step.FracMatrix);
	NewStep = step;
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

RowAndColumn CurrentLeadPos;
void ArtificialBasis(Step step, bool IsFractionalCoefficients) {
	static int PreviousStepID = step.StepID;

	if (step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	AlgorithmState state = UNDEFINED;
	if (IsFractionalCoefficients) {
		state = CheckAlgorithmState(step.FracMatrix, step.IsAutomatic, step.IsArtificialStep);
	} else {
		state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep);
	}

	if (state == UNLIMITED_SOLUTION) {
		printf("Solution is unlimited");
		step.IsCompleted = true;
	} else if (state == COMPLETED) {
		printf("Algorithm is completed");
		step.IsCompleted = true;
	} else if (state == SOLUTION_DOESNT_EXIST) {
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution doesn't exist!");
		printf("Solution doesn't exist");
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
		if (IsFractionalCoefficients) {
			std::vector<std::pair<Fraction, int>> MinimumAndRowIndex;
			for (int i = 0; i < step.FracMatrix.ColNumber - 1; i++) {
				CurrentRowIndex = -1;
				Fraction CurrentLead = Fraction(INT32_MAX, 1);
				Fraction ColumnMinimum = Fraction(INT32_MAX, 1);
				MinimumAndRowIndex.clear();
				if (step.FracMatrix[step.FracMatrix.RowNumber - 1][i] < 0) {
					for (int j = 0; j < step.FracMatrix.RowNumber - 1; j++) {
						if (step.FracMatrix[j][i] > 0) {
							if (step.FracMatrix[j][step.FracMatrix.ColNumber - 1] / step.FracMatrix[j][i] <= ColumnMinimum) {
								ColumnMinimum = step.FracMatrix[j][step.FracMatrix.ColNumber - 1] / step.FracMatrix[j][i];
								CurrentLead = step.FracMatrix[j][i];
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
						if (MR.first != ColumnMinimum) {
							MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
							i = -1;
						}
					}

					for (std::pair<Fraction, int> MR : MinimumAndRowIndex) {
						bool IsRowBanned = false;
						for (auto row : step.RowsBannedToSwap) {
							if (row == MR.second) {
								IsRowBanned = true;
							}
						}

						if (!IsRowBanned) {
							if (step.FracMatrix[step.FracMatrix.RowNumber - 1][i] < 0) {
								ImGui::PushID(i);
								ImGui::RadioButton((std::to_string(step.FracMatrix[CurrentRowIndex][i].numerator) + std::string("/") +
									std::to_string(step.FracMatrix[CurrentRowIndex][i].denominator)).c_str(), &Column, i); ImGui::SameLine();
								ImGui::PopID();
								Leads.push_back(RowAndColumn({ CurrentRowIndex, i }));
							}
						}
					}
				}
			}
		} else {
			std::vector<std::pair<float, int>> MinimumAndRowIndex;
			for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
				CurrentRowIndex = -1;
				float CurrentLead = FLT_MAX;
				float ColumnMinimum = FLT_MAX;
				MinimumAndRowIndex.clear();
				if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
					for (int j = 0; j < step.RealMatrix.RowNumber - 1; j++) {
						if (step.RealMatrix[j][i] > 0) {
							float TEMPDEBUG = step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i];
							if (step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i] <= ColumnMinimum + EPSILON) {
								ColumnMinimum = step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i];
								CurrentLead = step.RealMatrix[j][i];
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
						if (fabs(MR.first - ColumnMinimum) > EPSILON) {
							MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
							i = -1;
						}
					}

					for (std::pair<float, int> MR : MinimumAndRowIndex) {
						bool IsRowBanned = false;
						for (auto row : step.RowsBannedToSwap) {
							if (row == MR.second) {
								IsRowBanned = true;
							}
						}

						// Multiple Minimums
						if (!IsRowBanned) {
							int CellIndex = i + MR.second * step.RealMatrix.ColNumber;
							if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
								ImGui::PushID(CellIndex);
								ImGui::RadioButton(std::to_string(step.RealMatrix[MR.second][i]).c_str(), &CurrentCellIndex, CellIndex); ImGui::SameLine();
								ImGui::PopID();
								Leads.push_back(RowAndColumn({ MR.second, i }));
							}
						}
					}
				}
			}
		}

		// Solution doesn't exist if leads are empty
		if (!step.IsCompleted && Leads.size() == 0) {
			ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution doesn't exist!");
			step.IsCompleted = true;
		}

		if (!step.IsCompleted) {
			// Extract column nubmer out of cell index
			Column = CurrentCellIndex % step.RealMatrix.ColNumber;
			// Extract row number out of cell index
			int Row = CurrentCellIndex / step.RealMatrix.ColNumber;

			// Resets column to first available element each new step
			if (PreviousStepID != step.StepID) {
				Column = Leads[0].Column;
				Row = Leads[0].Row;
				PreviousStepID = step.StepID;
			}

			// Assign chosen row and column
			CurrentLeadPos.Column = Column;
			CurrentLeadPos.Row = Row;

			if (ImGui::Button("Confirm")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = Column;
				step.LeadElementRC.Row = Row;
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	}

	if (!step.IsWaitingForInput) {
		int RowNumber = 0;
		if (IsFractionalCoefficients) {
			RowNumber = step.FracMatrix.RowNumber;
		} else {
			RowNumber = step.RealMatrix.RowNumber;
		}
		for (int iteration = 0; iteration < step.RealMatrix.RowNumber; iteration++) {
			PrintMatrix(step.RealMatrix);

			Step NewStep = SimplexStep(step, IsFractionalCoefficients);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(RowNumber - 1) + NewStep.StepChosenRC.Column]);
			NewStep.NumbersOfVariables.erase(NewStep.NumbersOfVariables.begin() + (RowNumber - 1) + NewStep.StepChosenRC.Column);
			if (IsFractionalCoefficients) {
				NewStep.FracMatrix.DeleteColumn(NewStep.StepChosenRC.Column);
			} else {
				NewStep.RealMatrix.DeleteColumn(NewStep.StepChosenRC.Column);
			}

			// Disables "confirm" button
			if (IsFractionalCoefficients) {
				AlgorithmState state = CheckAlgorithmState(NewStep.FracMatrix, false, false);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			} else {
				AlgorithmState state = CheckAlgorithmState(NewStep.RealMatrix, false, false);
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

template<typename MatrixType, typename ElementType> void ArtificialBasis(Step step) {
	// Depending on type of matrix choose one of those to use
	MatrixType matrix;// = (IS_SAME_TYPE(MatrixType, Matrix)) ? step.RealMatrix : step.FracMatrix;
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		matrix = step.RealMatrix;
	} else {
		matrix = step.FracMatrix;
	}
	
	// Type-independent zero element
	// Float is [-EPSILON, +EPSILON], Fraction is 0/1
	ElementType ZeroElement;// = (IS_SAME_TYPE(MatrixType, Matrix)) ? EPSILON : Fraction(0, 1);
	if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
		ZeroElement = EPSILON;
	} else {
		ZeroElement = Fraction(0, 1);
	}

	static int PreviousStepID = step.StepID;

	if (step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	AlgorithmState state = UNDEFINED;
	state = CheckAlgorithmState(matrix, step.IsAutomatic, step.IsArtificialStep);

	if (state == UNLIMITED_SOLUTION) {
		printf("Solution is unlimited");
		step.IsCompleted = true;
	} else if (state == COMPLETED) {
		printf("Algorithm is completed");
		step.IsCompleted = true;
	} else if (state == SOLUTION_DOESNT_EXIST) {
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution doesn't exist!");
		printf("Solution doesn't exist");
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
			ElementType CurrentLead  ;// = (IS_SAME_TYPE(ElementType, float)) ? FLT_MAX : Fraction(INT32_MAX, 1);
			ElementType ColumnMinimum;// = (IS_SAME_TYPE(ElementType, float)) ? FLT_MAX : Fraction(INT32_MAX, 1);
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
					bool IsCurrentNotEqualToMinimum;// = (IS_SAME_TYPE(ElementType, float)) ? (fabs(MR.first - ColumnMinimum) > EPSILON) : (MR.first == ColumnMinimum);
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
					if (!IsRowBanned) {
						int CellIndex = i + MR.second * step.RealMatrix.ColNumber;
						if (matrix[matrix.RowNumber - 1][i] < 0) {
							ImGui::PushID(CellIndex);

							// Different formatting depending on which type is currently used
							if constexpr (IS_SAME_TYPE(ElementType, float)) {
								// Real case
								ImGui::RadioButton(std::to_string(matrix[MR.second][i]).c_str(), &CurrentCellIndex, CellIndex); ImGui::SameLine();
							} else {
								// Fractional case
								if (matrix[MR.second][i].denominator != 1) {
									// We display denominator if it doesn't equal to 1
									ImGui::RadioButton((std::to_string(matrix[MR.second][i].numerator) + std::string("/") + std::to_string(matrix[MR.second][i].denominator)).c_str(),
										&CurrentCellIndex, CellIndex); ImGui::SameLine();
								} else {
									// We don't display denominator if it equals to 1
									ImGui::RadioButton((std::to_string(matrix[MR.second][i].numerator)).c_str(), &CurrentCellIndex, CellIndex); ImGui::SameLine();
								}
							}

							ImGui::PopID();
							Leads.push_back(RowAndColumn({ MR.second, i }));
						}
					}
				}
			}
		}

		// Solution doesn't exist if leads are empty
		if (!step.IsCompleted && Leads.size() == 0) {
			ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution doesn't exist!");
			step.IsCompleted = true;
		}

		if (!step.IsCompleted) {
			// Extract column nubmer out of cell index
			Column = CurrentCellIndex % matrix.ColNumber;
			// Extract row number out of cell index
			int Row = CurrentCellIndex / matrix.ColNumber;

			// Resets column to first available element each new step
			if (PreviousStepID != step.StepID) {
				Column = Leads[0].Column;
				Row = Leads[0].Row;
				PreviousStepID = step.StepID;
			}

			// Assign chosen row and column
			CurrentLeadPos.Column = Column;
			CurrentLeadPos.Row = Row;

			if (ImGui::Button("Confirm")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = Column;
				step.LeadElementRC.Row = Row;
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	}

	if (!step.IsWaitingForInput) {
		int RowNumber = matrix.RowNumber;
		for (int iteration = 0; iteration < RowNumber; iteration++) {
			// Calculate current step
			Step NewStep = SimplexStep(step, false);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(RowNumber - 1) + NewStep.StepChosenRC.Column]);
			NewStep.NumbersOfVariables.erase(NewStep.NumbersOfVariables.begin() + (RowNumber - 1) + NewStep.StepChosenRC.Column);
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				NewStep.RealMatrix.DeleteColumn(NewStep.StepChosenRC.Column);
			} else {
				NewStep.FracMatrix.DeleteColumn(NewStep.StepChosenRC.Column);
			}

			// Disables "confirm" button
			if constexpr (IS_SAME_TYPE(ElementType, float)) {
				AlgorithmState state = CheckAlgorithmState(NewStep.RealMatrix, false, false);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}
			} else {
				AlgorithmState state = CheckAlgorithmState(NewStep.FracMatrix, false, false);
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

template<typename MatrixType> void DisplayStepOnScreen(MatrixType &matrix, int StepID, std::vector<int> NumbersOfVariables) {
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();

	ImGui::Columns(matrix.ColNumber + 1);

	ImGui::Text((std::string("#") + std::to_string(StepID)).c_str());
	ImGui::NextColumn();

	for (int i = 0; i < matrix.ColNumber - 1; i++) {
		ImGui::Text((std::string("x") + std::to_string(NumbersOfVariables[(matrix.RowNumber - 1) + i])).c_str());
		ImGui::NextColumn();
	}
	ImGui::NextColumn();
	ImGui::Separator();
	for (int i = 0; i < matrix.RowNumber; i++) {
		// Do not display variable name for the last row
		if (i < matrix.RowNumber - 1) {
			ImGui::Text((std::string("x") + std::to_string(NumbersOfVariables[i])).c_str());
			ImGui::NextColumn();
		}
		for (int j = 0; j < matrix.ColNumber; j++) {
			if (j == 0 && i == matrix.RowNumber - 1) {
				ImGui::NextColumn();
			}

			// Checking algorithm state
			AlgorithmState state = CheckAlgorithmState(matrix, false, false);

			// Fill with a color chosen cell
			if (state == CONTINUE && (i == CurrentLeadPos.Row) && (j == CurrentLeadPos.Column) && (StepID == ArtificialBasisSteps.size() - 2)) {
				float width = ImGui::GetColumnWidth();
				float height = ImGui::GetTextLineHeight();

				ImVec2 CursorPos = ImGui::GetCursorScreenPos();
				DrawList->AddRectFilled(ImVec2(CursorPos.x - 8.0f, CursorPos.y - 4.0f), ImVec2(CursorPos.x + width, CursorPos.y + height + 4.0f), IM_COL32(255 * 0.26f, 255 * 0.59f, 255 * 0.98f, 255));
			}

			// Output differences
			if constexpr (std::is_same<MatrixType, Matrix>::value) {
				// Real case
				ImGui::Text(std::to_string(matrix[i][j]).c_str());
			} else {
				// Fractional case
				if (matrix[i][j].denominator != 1) {
					// We display denominator if it doesn't equal to 1
					ImGui::Text((std::to_string(matrix[i][j].numerator) + std::string("/") + std::to_string(matrix[i][j].denominator)).c_str());
				} else {
					// We don't display denominator if it equals to 1
					ImGui::Text((std::to_string(matrix[i][j].numerator)).c_str());
				}
			}

			ImGui::NextColumn();
		}
		if (i != matrix.RowNumber - 1) {
			ImGui::Separator();
		}
	}

	ImGui::Columns(1);
}

void DisplaySteps(std::vector<Step> Steps, int StartIndex, bool IsFractionalCoefficients) {
	if (Steps.size() != 0) {
		if (ImGui::BeginTabBar((std::to_string(StartIndex) + std::string("Simplex Algorithm Solutions")).c_str())) {
			if (ImGui::BeginTabItem("Simplex Algorithm")) {
				for (int i = StartIndex; i < Steps.size(); i++) {
					if (IsFractionalCoefficients) {
						DisplayStepOnScreen(Steps[i].FracMatrix, Steps[i].StepID, Steps[i].NumbersOfVariables);
					} else {
						DisplayStepOnScreen(Steps[i].RealMatrix, Steps[i].StepID, Steps[i].NumbersOfVariables);
					}
					ImGui::Separator();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
}

void SimplexAlgorithm(Step step, bool IsFractionalCoefficients) {
	static int PreviousStepID = step.StepID;

	if (!step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	AlgorithmState state = UNDEFINED;
	if (IsFractionalCoefficients) {
		state = CheckAlgorithmState(step.FracMatrix, step.IsAutomatic, step.IsArtificialStep);
	} else {
		state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep);
	}
	assert(state != UNDEFINED);

	if (state == UNLIMITED_SOLUTION) {
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution is unlimited!");
		step.IsCompleted = true;
	} else if (state == COMPLETED) {
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Algorithm has completed!");
		step.IsCompleted = true;
	}

	// Choose lead element
	if (!step.IsAutomatic) {
		ImGui::PushID("Choose Lead Element");
		int CurrentRowIndex = -1;
		static int Column = 0;
		static int CurrentCellIndex = 0;

		// Pair of id and column number
		// TODO: Add multiple minimums support
		std::vector<RowAndColumn> Leads;
		if (IsFractionalCoefficients) {
			std::vector<std::pair<Fraction, int>> MinimumAndRowIndex;
			for (int i = 0; i < step.FracMatrix.ColNumber - 1; i++) {
				CurrentRowIndex = -1;
				Fraction CurrentLead = Fraction(INT32_MAX, 1);
				Fraction ColumnMinimum = Fraction(INT32_MAX, 1);
				MinimumAndRowIndex.clear();
				if (step.FracMatrix[step.FracMatrix.RowNumber - 1][i] < 0) {
					for (int j = 0; j < step.FracMatrix.RowNumber - 1; j++) {
						if (step.FracMatrix[j][i] > 0) {
							if (step.FracMatrix[j][step.FracMatrix.ColNumber - 1] / step.FracMatrix[j][i] < ColumnMinimum) {
								ColumnMinimum = step.FracMatrix[j][step.FracMatrix.ColNumber - 1] / step.FracMatrix[j][i];
								CurrentLead = step.FracMatrix[j][i];
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
						if (MR.first != ColumnMinimum) {
							MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
							i = -1;
						}
					}

					if (step.FracMatrix[step.FracMatrix.RowNumber - 1][i] < 0) {
						ImGui::PushID(i);
						ImGui::RadioButton((std::to_string(step.FracMatrix[CurrentRowIndex][i].numerator) +
							std::string("/") + std::to_string(step.FracMatrix[CurrentRowIndex][i].denominator)).c_str(), &Column, i); ImGui::SameLine();
						ImGui::PopID();
						// Push possible leading element
						Leads.push_back(RowAndColumn({ CurrentRowIndex, i }));
					}
				}
			}
		} else {
			std::vector<std::pair<float, int>> MinimumAndRowIndex;
			for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
				CurrentRowIndex = -1;
				float CurrentLead = FLT_MAX;
				float ColumnMinimum = FLT_MAX;
				MinimumAndRowIndex.clear();
				if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
					for (int j = 0; j < step.RealMatrix.RowNumber - 1; j++) {
						if (step.RealMatrix[j][i] > 0) {
							if (step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i] < ColumnMinimum) {
								ColumnMinimum = step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i];
								CurrentLead = step.RealMatrix[j][i];
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
						if (fabs(MR.first - ColumnMinimum) > EPSILON) {
							MinimumAndRowIndex.erase(MinimumAndRowIndex.begin() + i);
							i = -1;
						}
					}

					for (std::pair<float, int> MR : MinimumAndRowIndex) {
						// Multiple Minimums
						int CellIndex = i + MR.second * step.RealMatrix.ColNumber;
						if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
							ImGui::PushID(CellIndex);
							ImGui::RadioButton(std::to_string(step.RealMatrix[MR.second][i]).c_str(), &CurrentCellIndex, CellIndex); ImGui::SameLine();
							ImGui::PopID();
							Leads.push_back(RowAndColumn({ MR.second, i }));
						}
					}
				}
			}
		}

		if (!step.IsCompleted) {
			// Extract column nubmer out of cell index
			Column = CurrentCellIndex % step.RealMatrix.ColNumber;
			// Extract row number out of cell index
			int Row = CurrentCellIndex / step.RealMatrix.ColNumber;

			// Resets column to first available element each new step
			if (PreviousStepID != step.StepID || step.StepID == 0) {
				Column = Leads[0].Column;
				Row = Leads[0].Row;
				PreviousStepID = step.StepID;
			}

			// Assign chosen row and column
			CurrentLeadPos.Column = Column;
			CurrentLeadPos.Row = Row;

			if (ImGui::Button("Confirm")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = Column;
				step.LeadElementRC.Row = Row;
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	}

	if (!step.IsWaitingForInput) {
		int RowNumber = 0;
		int ColNumber = 0;
		// Set appropiate RowNumber and ColNumber according to whether fractional matrix or not 
		if (IsFractionalCoefficients) {
			RowNumber = step.FracMatrix.RowNumber;
			ColNumber = step.FracMatrix.ColNumber;
		} else {
			RowNumber = step.RealMatrix.RowNumber;
			ColNumber = step.RealMatrix.ColNumber;
		}


		for (int iteration = 0; iteration < RowNumber; iteration++) {
			PrintMatrix(step.RealMatrix);

			Step NewStep = SimplexStep(step, IsFractionalCoefficients);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(RowNumber - 1) + NewStep.StepChosenRC.Column]);

			// Disables "confirm" button
			if (IsFractionalCoefficients) {
				AlgorithmState state = CheckAlgorithmState(NewStep.FracMatrix, false, false);
				if (state == COMPLETED) {
					NewStep.IsCompleted = true;
				}

			} else {
				AlgorithmState state = CheckAlgorithmState(NewStep.RealMatrix, false, false);
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

void GaussElimination(Matrix& matrix) {
	int PivotRow = 0;
	int PivotColumn = 0;
	int num = 2;

	while (PivotRow < (matrix.RowNumber - 1) && PivotColumn < matrix.ColNumber - 1) {
		// Find max value in a column
		float MaxPivot = fabs(matrix[PivotRow][PivotColumn]);
		int MaxPivotIdx = PivotRow;
		for (int i = PivotRow; i < matrix.RowNumber - PivotRow - 1 - 1; i++) {
			if (fabs(matrix[i][PivotColumn]) > MaxPivot) {
				MaxPivot = fabs(matrix[i][PivotColumn]);
				MaxPivotIdx = i;
			}
		}

		if (fabs(matrix[MaxPivotIdx][PivotColumn]) < EPSILON) {
			PivotColumn += 1;
		} else {
			matrix.SwapRows(PivotRow, MaxPivotIdx);

			// Substract all rows below pivot
			for (int i = PivotRow + 1; i < matrix.RowNumber - 1; i++) {

				float Factor = matrix[i][PivotColumn] / matrix[PivotRow][PivotColumn];
				matrix[i][PivotColumn] = 0.0f;

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
		if (fabs(matrix[i][i]) < EPSILON) continue;
		for (int j = i; j > 0; j--) {
			float Factor = matrix[j - 1][i] / matrix[i][i];
			for (int k = i; k < matrix.ColNumber - 1; k++) {
				matrix[j - 1][k] -= matrix[i][k] * Factor;
			}

			matrix[j - 1][matrix.ColNumber - 1] -= matrix[i][matrix.ColNumber - 1] * Factor;
		}
	}

	// Normalization
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		float Pivot = matrix[i][i];
		if (fabs(Pivot) < EPSILON) continue;
		for (int j = i; j < matrix.ColNumber; j++) {
			matrix[i][j] /= Pivot;
		}
	}
}

Step ExplicitBasis(Step step, std::vector<float>& RealExplicitBasis, std::vector<float>& RealTargetFunction) {
	// 1. Prepare matrix to Gauss elimination
	std::vector<int> PositionsOfNonZeroElements;
	std::vector<int> VariablesPositions;
	for (int i = 0; i < RealExplicitBasis.size(); i++) {
		VariablesPositions.push_back(i + 1);
	}

	// 1.1 Find positions of non-zero elements
	for (int i = 0; i < RealExplicitBasis.size(); i++) {
		if (fabs(RealExplicitBasis[i]) > EPSILON) {
			PositionsOfNonZeroElements.push_back(i);
		}
	}

	// 1.2 Swap matrix column and variables to first (RowNumber) columns
	for (int i = 0; i < PositionsOfNonZeroElements.size(); i++) {
		int NonZeroPosition = PositionsOfNonZeroElements[i];
		step.RealMatrix.SwapColumns(i, NonZeroPosition);
		std::swap(VariablesPositions[i], VariablesPositions[NonZeroPosition]);
	}

	// 2. Gauss Elimination
	GaussElimination(step.RealMatrix);

	// 3. Complete table for first step of simplex alogrithm
	// 3.1 Delete first RowNumber columns
	for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
		step.RealMatrix.DeleteColumn(0);
	}

	step.NumbersOfVariables = VariablesPositions;
	MakeSimplexAlgorithmFunctionCoefficients(step, RealTargetFunction);
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
		ImGui::Begin("DockSpace Demo", 0, window_flags);
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

		ImGui::Separator();

		// Size is confirmed we can proceed further
		static int IsFractionalCoefficients = 0;
		if (SizeConfirmedReadyToContinue) {
			// Choose between real number and fractions
			ImGui::Text(u8"Выбор типа элементов");
			ImGui::RadioButton(u8"Действительные числа", &IsFractionalCoefficients, 0); ImGui::SameLine();
			ImGui::RadioButton(u8"Обыкновенные дроби", &IsFractionalCoefficients, 1);
			ImGui::Separator();

			// Choose between explicit and artificial basis
			ImGui::Text(u8"Выбор базиса");
			ImGui::RadioButton(u8"Явный базис", &IsArtificialBasis, 0); ImGui::SameLine();
			ImGui::RadioButton(u8"Искусственный базис", &IsArtificialBasis, 1);
			ImGui::Separator();

			// Choose between step by step and automatic solution modes
			ImGui::Text(u8"Режим решения");
			ImGui::RadioButton(u8"Пошаговый", &IsAutomatic, 0);
			ImGui::SameLine(); ImGui::RadioButton(u8"Автоматический", &IsAutomatic, 1);
			ImGui::Separator();

			// Apply all properties and continue
			ImGui::PushID("Properties Apply");
			if (ImGui::Button(u8"Применить")) {
				ContinueToProblemInput = true;
			}
			ImGui::PopID();

			ImGui::Separator();
		}
		ImGui::End();

		// Data input window
		ImGui::Begin(u8"Ввод данных", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
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
							ImGui::NextColumn();
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

			// ====================================
			//		VERIFIED UNTIL THAT POINT
			// ====================================

			ImGui::Spacing();

			if (ImGui::Button(u8"Решить")) {
				ShowSolution = true;
			}
			
			if (ShowSolution) {
				ImGui::SameLine();
				if (ImGui::Button(u8"Отменить решение")) {
					Step FirstStep = ArtificialBasisSteps[0];
					ArtificialBasisSteps.clear();
					ArtificialBasisSteps.push_back(FirstStep);
					SimplexAlgorithmSteps.clear();
					ShowSolution = false;
					StartSimplexAlgorithm = false;
				}
			}

			ImGui::Separator();
			ImGui::Text("Solution");

			if (ShowSolution) {
				// Artificial Basis Step
				if (IsArtificialBasis) {
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
					DisplaySteps(ArtificialBasisSteps, 1, IsFractionalCoefficients);
					//ArtificialBasis(step, IsFractionalCoefficients);
					if (!IsFractionalCoefficients) {
						ArtificialBasis<Matrix, float>(step);
					} else {
						ArtificialBasis<FractionalMatrix, Fraction>(step);
					}
				} else {
					// Explicit Basis Steps
					// TODO: Make explicit basis steps for rational coefficients
					if (SimplexAlgorithmSteps.size() == 0) {
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

						step = ExplicitBasis(step, RealExplicitBasis, RealTargetFunction);
						step.IsCompleted = true;
						//ExplicitBasisSteps.push_back(step);
						DisplaySteps(ExplicitBasisSteps, 0, IsFractionalCoefficients);
					}

					if (SimplexAlgorithmSteps.size() != 0) {
						//DisplaySteps(ExplicitBasisSteps, 0);
					}
				}

				if (step.IsCompleted) {
					if (ImGui::Button(u8"Start Simplex Algorithm")) {
						StartSimplexAlgorithm = true;
					}
				}

				if (StartSimplexAlgorithm) {
					if (SimplexAlgorithmSteps.size() == 0) {
						if (IsArtificialBasis) {
							size_t LastElementIndex = ArtificialBasisSteps.size() - 1;
							step.RealMatrix = ArtificialBasisSteps[LastElementIndex].RealMatrix;
							step.FracMatrix = ArtificialBasisSteps[LastElementIndex].FracMatrix;
							step.NumbersOfVariables = ArtificialBasisSteps[LastElementIndex].NumbersOfVariables;
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
					DisplaySteps(SimplexAlgorithmSteps, 0, IsFractionalCoefficients);
					SimplexAlgorithm(step, IsFractionalCoefficients);
				}
			}
		}

		ImGui::End();

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