/*
		TODO: Save values of matrix on resize
		TODO: [Save values to file] and restore values from file
		TODO: Calculate matrix for explicit basis
		[] Возможность решения задачи с использованием заданных базисных переменных.
		[] Работа с обыкновенными и десятичными дробями.
		[] Контроль данных (защита от «дурака»)
		[] [Сохранение введённой задачи в файл] и чтение из файла.
		[] В пошаговом режиме возможность возврата назад.
		[] Справка.
		[] Контекстно-зависимая помощь.
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

#include "Common.h"

static int Clamp(int value, int min, int max) {
	if (value > max) return max;
	if (value < min) return min;

	return value;
}

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

AlgorithmState CheckAlgorithmState(Matrix &matrix, bool IsAutomatic, bool IsArtificialStep, int *CurrentColumnIndex) {
	AlgorithmState state = UNDEFINED;
	
	for (int i = 0; i < matrix.ColNumber - 1; i++) {
		if (matrix[matrix.RowNumber - 1][i] < -EPSILON) {
			// Check if there is at least one positive element in a column
			for (int j = 0; j < matrix.RowNumber - 1; j++) {
				if (matrix[j][i] > EPSILON) {
					state = CONTINUE;
					if (IsAutomatic) {
						assert(CurrentColumnIndex != NULL);
						*CurrentColumnIndex = i;
					}
					break;
				}
			}
			if (state == UNDEFINED) {
				state = UNLIMITED_SOLUTION;
			}
		}
	}

	if (state == UNDEFINED) {
		state = COMPLETED;
	}

	assert(state != UNDEFINED);
	return state;
}

Step SimplexStep(Step step) {
	int CurrentColumnIndex = -1;
	int CurrentRowIndex = -1;
	float CurrentLead = -1;
	
	AlgorithmState state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep, &CurrentColumnIndex);

	if (state == UNLIMITED_SOLUTION) {
		printf("Solution is unlimited");
		step.IsCompleted = true;
		return step;
	} else if (state == COMPLETED) {
		printf("Algorithm is completed");
		step.IsCompleted = true;
		return step;
	}
	
	// Choose lead element
	if (step.IsAutomatic) {
		CurrentRowIndex = -1;
		CurrentLead = FLT_MAX;
		float ColumnMinimum = FLT_MAX;

		// Choose any available lead element
		for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
			if (step.RealMatrix[i][CurrentColumnIndex] > EPSILON) {
				assert(step.RealMatrix[i][CurrentColumnIndex] > EPSILON);
				if (step.RealMatrix[i][step.RealMatrix.ColNumber - 1] / step.RealMatrix[i][CurrentColumnIndex] < ColumnMinimum) {
					CurrentLead = step.RealMatrix[i][CurrentColumnIndex];
					ColumnMinimum = step.RealMatrix[i][step.RealMatrix.ColNumber - 1] / step.RealMatrix[i][CurrentColumnIndex];
					CurrentRowIndex = i;
				}
			}
		}
	} else {
		CurrentColumnIndex = step.LeadElementRC.Column;
		CurrentRowIndex = step.LeadElementRC.Row;
		CurrentLead = step.RealMatrix[step.LeadElementRC.Row][step.LeadElementRC.Column];
	}

	assert(CurrentRowIndex != -1);
	assert(CurrentColumnIndex != -1);
	assert(CurrentLead != -1);

	// Lead element is equal to 1 / Lead
	step.RealMatrix[CurrentRowIndex][CurrentColumnIndex] = 1 / CurrentLead;

	// Divide Row by lead
	for (int i = 0; i < step.RealMatrix.ColNumber; i++) {
		if (i == CurrentColumnIndex) { continue; }
		step.RealMatrix[CurrentRowIndex][i] /= CurrentLead;
	}

	// Divide Column by negative lead
	for (int i = 0; i < step.RealMatrix.RowNumber; i++) {
		if (i == CurrentRowIndex) { continue; }
		step.RealMatrix[i][CurrentColumnIndex] /= -CurrentLead;
	}

	// Subtract all other rows by lead row
	for (int i = 0; i < step.RealMatrix.RowNumber; i++) {
		if (i == CurrentRowIndex) { continue; }
		for (int j = 0; j < step.RealMatrix.ColNumber; j++) {
			if (j == CurrentColumnIndex) { continue; }
			step.RealMatrix[i][j] -= (-1) * CurrentLead * step.RealMatrix[i][CurrentColumnIndex] * step.RealMatrix[CurrentRowIndex][j];
		}
	}

	Step NewStep(step.RealMatrix);
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

void MakeSimplexAlgorithmFunctionCoefficients(Step& step, std::vector<float> &RealTargetFunction) {
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

RowAndColumn CurrentLeadPos;
void ArtificialBasis(Step step) {
	if (step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	// Choose lead element
	if (!step.IsAutomatic) {
		ImGui::PushID("Choose Lead Element");
		int CurrentRowIndex = -1;
		int CurrentColumnIndex = -1;
		static int Column = 0;
		// Pair of id and column number
		std::vector<RowAndColumn> Leads;
		for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
			CurrentRowIndex = -1;
			float CurrentLead = FLT_MAX;
			float ColumnMinimum = FLT_MAX;
			if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
				for (int j = 0; j < step.RealMatrix.RowNumber - 1; j++) {
						if (step.RealMatrix[j][i] > 0) {
							if (step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i] < ColumnMinimum) {	
								ColumnMinimum = step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i];
								CurrentLead = step.RealMatrix[j][i];
								CurrentRowIndex = j;
							}
						}
				}
			}

			if (CurrentRowIndex != -1) {
				bool IsRowBanned = false;
				for (auto row : step.RowsBannedToSwap) {
					if (row == CurrentRowIndex) {
						IsRowBanned = true;
					}
				}

				if (!IsRowBanned) {
					if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
						ImGui::PushID(i);
						ImGui::RadioButton(std::to_string(step.RealMatrix[CurrentRowIndex][i]).c_str(), &Column, i); ImGui::SameLine();
						ImGui::PopID();
						Leads.push_back(RowAndColumn({ CurrentRowIndex, i }));
					}
				}
			}

			AlgorithmState state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep, &CurrentColumnIndex);
			if (state == UNLIMITED_SOLUTION) {
				printf("Solution is unlimited");
				step.IsCompleted = true;
			} else if (state == COMPLETED) {
				printf("Algorithm is completed");
				step.IsCompleted = true;
			}
		}

		if (!step.IsCompleted) {
			// Assign chosen row and column
			CurrentLeadPos.Column = Column;
			for (auto rc : Leads) {
				if (rc.Column == Column) {
					CurrentLeadPos.Row = rc.Row;
				}
			}

			if (ImGui::Button("Confirm")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = Column;
				for (RowAndColumn rc : Leads) {
					if (rc.Column == Column) {
						step.LeadElementRC.Row = rc.Row;
					}
				}
				assert(step.LeadElementRC.Row    != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	}

	if (!step.IsWaitingForInput) {
		for (int iteration = 0; iteration < step.RealMatrix.RowNumber; iteration++) {
			PrintMatrix(step.RealMatrix);

			Step NewStep = SimplexStep(step);
			
			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(NewStep.RealMatrix.RowNumber - 1) + NewStep.StepChosenRC.Column]);
			NewStep.NumbersOfVariables.erase(NewStep.NumbersOfVariables.begin() + (NewStep.RealMatrix.RowNumber - 1) + NewStep.StepChosenRC.Column);
			NewStep.RealMatrix.DeleteColumn(NewStep.StepChosenRC.Column);
			printf("%d) Col: %d Row: %d\n", iteration, NewStep.StepChosenRC.Column, NewStep.StepChosenRC.Row);
			PrintMatrix(NewStep.RealMatrix);

			// Disables "confirm" button
			if (fabs(NewStep.RealMatrix[NewStep.RealMatrix.RowNumber - 1][NewStep.RealMatrix.ColNumber - 1]) < EPSILON) {
				NewStep.IsCompleted = true;
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

void DisplayStepOnScreen(Step &step) {
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	
	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();

	ImGui::Columns(step.RealMatrix.ColNumber + 1);
	
	ImGui::Text((std::string("#") + std::to_string(step.StepID)).c_str());
	ImGui::NextColumn();

	for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
		ImGui::Text((std::string("x") + std::to_string(step.NumbersOfVariables[(step.RealMatrix.RowNumber - 1) + i])).c_str());
		ImGui::NextColumn();
	}
	ImGui::NextColumn();
	ImGui::Separator();
	for (int i = 0; i < step.RealMatrix.RowNumber; i++) {
		// Do not display variable name for the last row
		if (i < step.RealMatrix.RowNumber - 1) {
			ImGui::Text((std::string("x") + std::to_string(step.NumbersOfVariables[i])).c_str());
			ImGui::NextColumn();
		}
		for (int j = 0; j < step.RealMatrix.ColNumber; j++) {
			if (j == 0 && i == step.RealMatrix.RowNumber - 1) {
				ImGui::NextColumn();
			}

			// Fill with a color chosen cell
			// BUG: Rectangles move whenever scrolling is happening
			if (!step.IsCompleted && i == CurrentLeadPos.Row && j == CurrentLeadPos.Column && step.StepID == ArtificialBasisSteps.size() - 2) {
				float width = ImGui::GetColumnWidth();
				float height = ImGui::GetTextLineHeight();

				ImVec2 CursorPos = ImGui::GetCursorPos();
				DrawList->AddRectFilled(ImVec2(CursorPos.x - 8.0f, CursorPos.y - 4.0f), ImVec2(CursorPos.x + width, CursorPos.y + height + 4.0f), IM_COL32(255 * 0.26f, 255 * 0.59f, 255 * 0.98f, 255));
			}
			
			ImGui::Text(std::to_string(step.RealMatrix[i][j]).c_str());
			ImGui::NextColumn();
		}
		if (i != step.RealMatrix.RowNumber - 1) {
			ImGui::Separator();
		}
	}
	ImGui::Columns(1);
}

void DisplayAllArtificialSteps() {
	if (ArtificialBasisSteps.size() != 0) {
		if (ImGui::BeginTabBar("Solutions")) {
			if (ImGui::BeginTabItem("Artificial Basis")) {
				// First element skipped
				for (int i = 1; i < ArtificialBasisSteps.size(); i++) {
					DisplayStepOnScreen(ArtificialBasisSteps[i]);
					ImGui::Separator();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
}

void DisplayAllSimplexAlgorithmSteps() {
	if (SimplexAlgorithmSteps.size() != 0) {
		if (ImGui::BeginTabBar("Simplex Algorithm Solutions")) {
			if (ImGui::BeginTabItem("Simplex Algorithm")) {
				for (int i = 0; i < SimplexAlgorithmSteps.size(); i++) {
					DisplayStepOnScreen(SimplexAlgorithmSteps[i]);
					ImGui::Separator();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
}

void DisplaySteps(std::vector<Step> Steps, int StartIndex) {
	if (Steps.size() != 0) {
		if (ImGui::BeginTabBar("Simplex Algorithm Solutions")) {
			if (ImGui::BeginTabItem("Simplex Algorithm")) {
				for (int i = StartIndex; i < Steps.size(); i++) {
					DisplayStepOnScreen(Steps[i]);
					ImGui::Separator();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
}

void SimplexAlgorithm(Step step) {
	if (!step.IsArtificialStep && step.IsCompleted) {
		return;
	}

	// Choose lead element
	if (!step.IsAutomatic) {
		ImGui::PushID("Choose Lead Element");
		int CurrentRowIndex = -1;
		int CurrentColumnIndex = -1;
		static int Column = 0;
		// Pair of id and column number
		std::vector<RowAndColumn> Leads;
		for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
			CurrentRowIndex = -1;
			float CurrentLead = FLT_MAX;
			float ColumnMinimum = FLT_MAX;
			if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
				for (int j = 0; j < step.RealMatrix.RowNumber - 1; j++) {
					if (step.RealMatrix[j][i] > 0) {
						if (step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i] < ColumnMinimum) {
							ColumnMinimum = step.RealMatrix[j][step.RealMatrix.ColNumber - 1] / step.RealMatrix[j][i];
							CurrentLead = step.RealMatrix[j][i];
							CurrentRowIndex = j;
						}
					}
				}
			}

			if (CurrentRowIndex != -1) {
				Leads.push_back(RowAndColumn({ CurrentRowIndex, i }));
				if (step.RealMatrix[step.RealMatrix.RowNumber - 1][i] < 0) {
					ImGui::PushID(i);
					ImGui::RadioButton(std::to_string(step.RealMatrix[CurrentRowIndex][i]).c_str(), &Column, i); ImGui::SameLine();
					ImGui::PopID();
				}
			}

			AlgorithmState state = CheckAlgorithmState(step.RealMatrix, step.IsAutomatic, step.IsArtificialStep, &CurrentColumnIndex);
			if (state == UNLIMITED_SOLUTION) {
				ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Solution is unlimited!");
				step.IsCompleted = true;
			} else if (state == COMPLETED) {
				ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Algorithm has completed!");
				step.IsCompleted = true;
			}
		}

		if (!step.IsCompleted) {
			// Assign chosen row and column
			CurrentLeadPos.Column = Column;
			for (auto rc : Leads) {
				if (rc.Column == Column) {
					CurrentLeadPos.Row = rc.Row;
				}
			}

			if (ImGui::Button("Confirm")) {
				assert(Column != -1);
				step.IsWaitingForInput = false;
				step.LeadElementRC.Column = Column;
				for (RowAndColumn rc : Leads) {
					if (rc.Column == Column) {
						step.LeadElementRC.Row = rc.Row;
					}
				}
				assert(step.LeadElementRC.Row != -1);
				assert(step.LeadElementRC.Column != -1);
			}
		}

		ImGui::PopID();
	}

	if (!step.IsWaitingForInput) {
		for (int iteration = 0; iteration < step.RealMatrix.RowNumber; iteration++) {
			PrintMatrix(step.RealMatrix);

			Step NewStep = SimplexStep(step);

			if (NewStep.IsCompleted) {
				break;
			}

			// Change order of variables in the array of variables
			std::swap(NewStep.NumbersOfVariables[NewStep.StepChosenRC.Row], NewStep.NumbersOfVariables[(NewStep.RealMatrix.RowNumber - 1) + NewStep.StepChosenRC.Column]);
			printf("%d) Col: %d Row: %d\n", iteration, NewStep.StepChosenRC.Column, NewStep.StepChosenRC.Row);
			PrintMatrix(NewStep.RealMatrix);

			// Disables "confirm" button
			if (fabs(NewStep.RealMatrix[NewStep.RealMatrix.RowNumber - 1][NewStep.RealMatrix.ColNumber - 1]) < EPSILON) {
				NewStep.IsCompleted = true;
			}

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
	// BUG: Coefficients have opposite sign as they should have 
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
	std::vector<Fraction> FractionalTargetFunction(1);
	std::vector<float> RealTargetFunction(1);
	std::vector<Fraction> FractionalExplicitBasis(1);
	std::vector<float> RealExplicitBasis(1);
	Matrix RealMatrix(1, 1);
	FractionalMatrix FractionalMatrix(1, 1);
	Step step(RealMatrix);
	ArtificialBasisSteps.push_back(step);

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Simplex Algorithm", 0, 0);
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

	ImGui::StyleColorsLight();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430 core");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Simplex algorithm", NULL, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::SetWindowPos(ImVec2(0, 0));

		ImGui::PushID(1);
		ImGui::Text("Number of Variables  "); ImGui::SameLine();
		ImGui::SetNextItemWidth(35);
		ImGui::InputScalar("", ImGuiDataType_U8, &NumberOfVariables);
		ImGui::PopID();

		NumberOfVariables = Clamp(NumberOfVariables, 1, 16);

		ImGui::PushID(2);
		ImGui::Text("Number of Limitations"); ImGui::SameLine();
		ImGui::SetNextItemWidth(35);
		ImGui::InputScalar("", ImGuiDataType_U8, &NumberOfLimitations);
		ImGui::PopID();

		NumberOfLimitations = Clamp(NumberOfLimitations, 1, 16);

		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			SizeConfirmedClicked++;
		}

		ImGui::Separator();

		static int IsFractionalCoefficients = 0;
		ImGui::RadioButton("Real", &IsFractionalCoefficients, 0); ImGui::SameLine();
		ImGui::RadioButton("Fractions", &IsFractionalCoefficients, 1);

		ImGui::Separator();

		ImGui::RadioButton("Explicit Basis", &IsArtificialBasis, 0); ImGui::SameLine();
		ImGui::RadioButton("Artificial Basis", &IsArtificialBasis, 1);

		if (!IsArtificialBasis) {
			ImGui::PushID("Explicit Basis");
			if (!IsFractionalCoefficients) {
				ImGui::PushID("Real Basis");
				for (int i = 0; i < RealMatrix.ColNumber - 1; i++) {
					ImGui::PushID(i);
					ImGui::SetNextItemWidth(75);
					ImGui::InputScalar("", ImGuiDataType_Float, &RealExplicitBasis.at(i));
					if (i < RealMatrix.ColNumber - 1) { ImGui::SameLine(); }
					ImGui::PopID();
				}
				ImGui::PopID();
			} else {
				ImGui::PushID("Fractional Basis");
				ImGui::PushID("Numerator");
				for (int i = 0; i < FractionalMatrix.ColNumber - 1; i++) {
					ImGui::PushID(i);
					ImGui::SetNextItemWidth(75);
					ImGui::InputScalar("", ImGuiDataType_S32, &FractionalExplicitBasis.at(i).numerator);
					if (i < FractionalMatrix.ColNumber - 1) { ImGui::SameLine(); }
					ImGui::PopID();
				}
				ImGui::PopID();

				ImGui::PushID("Denominator");
				for (int i = 0; i < FractionalMatrix.ColNumber - 1; i++) {
					ImGui::PushID(i);
					ImGui::SetNextItemWidth(75);
					ImGui::InputScalar("", ImGuiDataType_U32, &FractionalExplicitBasis.at(i).denominator);
					FractionalExplicitBasis.at(i).denominator = Clamp(FractionalExplicitBasis.at(i).denominator, 1, INT32_MAX);
					if (i < FractionalMatrix.ColNumber - 1) { ImGui::SameLine(); }
					ImGui::PopID();
				}
				ImGui::PopID();
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		ImGui::Separator();

		ImGui::Text("Mode");
		ImGui::RadioButton("Step by step", &IsAutomatic, 0);
		ImGui::SameLine(); ImGui::RadioButton("Automatic", &IsAutomatic, 1);

		ImGui::Separator();

		ImGui::PushID("Matrix");
		ImGui::Text("Matrix of Limitations");

		// Resize
		if (SizeConfirmedClicked != OldSizeCondfirmedClicked) {
			OldSizeCondfirmedClicked++;
			if (!IsFractionalCoefficients) {
				// +1 for function coefficients			   // +1 for vector B
				RealMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables + 1);

				// UNDONE: +1 if there is a constant
				RealTargetFunction.resize(NumberOfVariables + 1);
				RealExplicitBasis.resize(NumberOfVariables);
			} else {
				// UNDONE: +1 for function coefficients
				FractionalMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables + 1);
				// UNDONE: +1 if there is a constant
				FractionalTargetFunction.resize(NumberOfVariables + 1);
				FractionalExplicitBasis.resize(NumberOfVariables);
			}
		}

		if (!IsFractionalCoefficients) {
			for (int i = 0; i < RealMatrix.RowNumber - 1; i++) {
				for (int j = 0; j < RealMatrix.ColNumber; j++) {
					ImGui::PushID(i * RealMatrix.ColNumber + j);

					ImGui::SetNextItemWidth(75);
					ImGui::InputScalar("", ImGuiDataType_Float, &RealMatrix[i][j]);
					if (j < RealMatrix.ColNumber - 1) { ImGui::SameLine(); }

					ImGui::PopID();
				}
			}
		} else {
			for (int i = 0; i < FractionalMatrix.RowNumber; i++) {
				ImGui::NewLine();
				ImGui::PushID("Numerator");
				for (int j = 0; j < FractionalMatrix.ColNumber; j++) {
					ImGui::PushID(i * FractionalMatrix.ColNumber + j);
					ImGui::SetNextItemWidth(40);
					ImGui::InputScalar("", ImGuiDataType_S32, &FractionalMatrix[i][j].numerator);
					if (j < FractionalMatrix.ColNumber - 1) { ImGui::SameLine(); }
					ImGui::PopID();
				}
				ImGui::PopID();

				ImGui::PushID("Denominator");
				for (int j = 0; j < FractionalMatrix.ColNumber; j++) {
					ImGui::PushID(i * FractionalMatrix.ColNumber + j);
					ImGui::SetNextItemWidth(40);
					ImGui::InputScalar("", ImGuiDataType_U32, &FractionalMatrix[i][j].denominator);
					if (j < FractionalMatrix.ColNumber - 1) { ImGui::SameLine(); }
					FractionalMatrix[i][j].denominator = Clamp(FractionalMatrix[i][j].denominator, 1, INT32_MAX);
					ImGui::PopID();
				}
				ImGui::PopID();
			}
		}
		ImGui::PopID();

		ImGui::Separator();

		ImGui::Text("Target Function");

		if (!IsFractionalCoefficients) {
			ImGui::PushID("Real Function");
			for (int i = 0; i < RealTargetFunction.size(); i++) {
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(75);
				ImGui::InputScalar("", ImGuiDataType_Float, &RealTargetFunction.at(i));
				if (i != RealTargetFunction.size() - 1) {
					ImGui::SameLine(); ImGui::Text((std::string("x") + std::to_string(i + 1) + std::string(" +")).c_str());
				} else {
					ImGui::SameLine(); ImGui::Text(std::string(" -> min").c_str());
				}
				if (i < RealTargetFunction.size() - 1) { ImGui::SameLine(); }
				ImGui::PopID();
			}
			ImGui::PopID();
		} else {
			ImGui::PushID("Fractional Function");

			ImGui::PushID("Numerator");
			for (int i = 0; i < FractionalTargetFunction.size(); i++) {
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(60);
				ImGui::InputScalar("", ImGuiDataType_S32, &FractionalTargetFunction.at(i).numerator);
				if (i != FractionalTargetFunction.size() - 1) {
					ImGui::SameLine(); ImGui::Text((std::string("x") + std::to_string(i) + std::string(" +")).c_str());
				} else {
					ImGui::SameLine(); ImGui::Text((std::string("x") + std::to_string(i) + std::string(" -> min")).c_str());
				}
				if (i < FractionalTargetFunction.size() - 1) { ImGui::SameLine(); }
				ImGui::PopID();
			}
			ImGui::PopID();

			ImGui::PushID("Denominator");
			for (int i = 0; i < FractionalTargetFunction.size(); i++) {
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(60);
				ImGui::InputScalar("", ImGuiDataType_U32, &FractionalTargetFunction.at(i).denominator);
				if (i < 10) {
					ImGui::SameLine(); ImGui::Text("    ");
				} else {
					ImGui::SameLine(); ImGui::Text("     ");
				}
				if (i < FractionalTargetFunction.size() - 1) { ImGui::SameLine(); }
				FractionalTargetFunction.at(i).denominator = Clamp(FractionalTargetFunction.at(i).denominator, 1, INT32_MAX);
				ImGui::PopID();
			}
			ImGui::PopID();

			ImGui::PopID();
		}

		ImGui::Spacing();

		if (ImGui::Button("Solve")) {
			ShowSolution = true;
		}
		if (ImGui::Button("Cancel")) {
			Step FirstStep = ArtificialBasisSteps[0];
			ArtificialBasisSteps.clear();
			ArtificialBasisSteps.push_back(FirstStep);
			SimplexAlgorithmSteps.clear();
			ShowSolution = false;
			StartSimplexAlgorithm = false;
		}

		ImGui::Separator();
		ImGui::Text("Solution");

		if (ShowSolution) {
			if (IsArtificialBasis) {
				// Calculate next step of simplex algorithm
				size_t LastElementIndex = ArtificialBasisSteps.size() - 1;
				step = ArtificialBasisSteps[LastElementIndex];
				if (ArtificialBasisSteps.size() == 1) {
					step.RealMatrix = RealMatrix;
					step.IsAutomatic = IsAutomatic;
					step.IsCompleted = false;
					step.IsArtificialStep = true;
					// Artificial variables
					for (int i = 0; i < step.RealMatrix.RowNumber - 1; i++) {
						step.NumbersOfVariables.push_back(step.RealMatrix.ColNumber + i);
					}
					// Variables
					for (int i = 0; i < step.RealMatrix.ColNumber - 1; i++) {
						step.NumbersOfVariables.push_back(i + 1);
					}
					MakeArtificialFunctionCoefficients(step.RealMatrix);
					ArtificialBasisSteps.push_back(step);
				}

				if (!step.IsAutomatic) {
					step.IsWaitingForInput = true;
				} else {
					step.IsWaitingForInput = false;
				}

				// Display all steps that has been calculated
				DisplayAllArtificialSteps();
				ArtificialBasis(step);
			} else {
				// TODO: Explicit basis
				if (SimplexAlgorithmSteps.size() == 0) {
					step.RealMatrix = RealMatrix;
					step.IsAutomatic = IsAutomatic;
					step.IsCompleted = false;
					step.IsArtificialStep = true;
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
					DisplaySteps(ExplicitBasisSteps, 0);
				}
				
				if (SimplexAlgorithmSteps.size() != 0) {
					//DisplaySteps(ExplicitBasisSteps, 0);
				}
			}

			if (step.IsCompleted) {
				if (ImGui::Button("Start Simplex Algorithm")) {
					StartSimplexAlgorithm = true;
				}
			}

			if (StartSimplexAlgorithm) {
				if (SimplexAlgorithmSteps.size() == 0) {
					if (IsArtificialBasis) {
						size_t LastElementIndex = ArtificialBasisSteps.size() - 1;
						step.RealMatrix = ArtificialBasisSteps[LastElementIndex].RealMatrix;
						step.NumbersOfVariables = ArtificialBasisSteps[LastElementIndex].NumbersOfVariables;
						MakeSimplexAlgorithmFunctionCoefficients(step, RealTargetFunction);
					}

					step.IsAutomatic = IsAutomatic;
					step.IsCompleted = false;
					step.IsArtificialStep = false;

					SimplexAlgorithmSteps.push_back(step);
				} else {
					size_t LastSimplexAlgorithmElementIndex = SimplexAlgorithmSteps.size() - 1;
					step = SimplexAlgorithmSteps[LastSimplexAlgorithmElementIndex];
				}
				DisplayAllSimplexAlgorithmSteps();
				SimplexAlgorithm(step);
			}

			// SAVING TO FILE
			if (ImGui::Button("Save to file")) {
				FILE* file = fopen("ProblemConfiguration.txt", "w");
				if (!file) { printf("Cannot open a file!\n"); }

				/*
					FILE FORMAT:
						NV: <number> - number of variables
						NL: <number> - number of limitations
						F/R - fractional/real
						E/A - explicit basis/artificial basis
						ML: - Matrix of limitations
								1.223 -1.23
								0.232 0.14
						TF:	Target function
				*/

				if (!IsFractionalCoefficients) {
					std::string nv = (std::string("NV: ") + std::to_string(RealMatrix.ColNumber) + std::string("\n"));
					fprintf(file, nv.c_str());
					std::string nl = (std::string("NL: ") + std::to_string(RealMatrix.RowNumber) + std::string("\n"));
					fprintf(file, nl.c_str());
					fprintf(file, "R\n");

					if (IsArtificialBasis) {
						fprintf(file, "A\n");
					} else {
						fprintf(file, "E:\n");
						for (int i = 0; i < RealExplicitBasis.size(); i++) {
							fprintf(file, std::to_string(RealExplicitBasis[i]).c_str());
							fprintf(file, " ");
						}
						fprintf(file, "\n");
					}

					fprintf(file, "ML:\n");
					for (int i = 0; i < RealMatrix.RowNumber; i++) {
						for (int j = 0; j < RealMatrix.ColNumber; j++) {
							fprintf(file, std::to_string(RealMatrix[i][j]).c_str());
							fprintf(file, " ");
						}
						fprintf(file, "\n");
					}
					fprintf(file, "TF:\n");
					for (int i = 0; i < RealTargetFunction.size(); i++) {
						fprintf(file, std::to_string(RealTargetFunction[i]).c_str());
						fprintf(file, " ");
					}
				} else {
					std::string nv = (std::string("NV: ") + std::to_string(FractionalMatrix.ColNumber) + std::string("\n"));
					fprintf(file, nv.c_str());
					std::string nl = (std::string("NL: ") + std::to_string(FractionalMatrix.RowNumber) + std::string("\n"));
					fprintf(file, nl.c_str());
					fprintf(file, "F\n");

					if (IsArtificialBasis) {
						fprintf(file, "A\n");
					} else {
						fprintf(file, "E:\n");
						for (int i = 0; i < FractionalExplicitBasis.size(); i++) {
							fprintf(file, (std::to_string(FractionalExplicitBasis[i].numerator) + std::string("/") + std::to_string(FractionalExplicitBasis[i].denominator)).c_str());
							fprintf(file, " ");
						}
						fprintf(file, "\n");
					}

					fprintf(file, "ML:\n");
					for (int i = 0; i < FractionalMatrix.RowNumber; i++) {
						for (int j = 0; j < FractionalMatrix.ColNumber; j++) {
							fprintf(file, (std::to_string(FractionalMatrix[i][j].numerator) + std::string("/") + std::to_string(FractionalMatrix[i][j].denominator)).c_str());
							fprintf(file, " ");
						}
						fprintf(file, "\n");
					}
					fprintf(file, "TF:\n");
					for (int i = 0; i < FractionalTargetFunction.size(); i++) {
						fprintf(file, (std::to_string(FractionalTargetFunction[i].numerator) + std::string("/") + std::to_string(FractionalTargetFunction[i].denominator)).c_str());
						fprintf(file, " ");
					}
				}
				fclose(file);
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