#pragma once

namespace GUILayer {

const char* ComboBoxVariablesLabel = u8"Число переменных ";
const char* ComboBoxLimitationsLabel = u8"Число ограничений";

int ComboBox(const char *label) {
	const char* values[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" };
	static int CallNumber = 0;
	CallNumber = CallNumber == 0 ? 1 : 0;

	// Align labels of comboboxes
	ImGui::Text(label); ImGui::SameLine();
	ImGui::SetNextItemWidth(75);

	static int CallOneCurrentItem = 0;
	static int CallTwoCurrentItem = 0;
	
	static int *CurrentItem = NULL;
	if (CallNumber == 0) {
		CurrentItem = &CallOneCurrentItem;
	} else {
		CurrentItem = &CallTwoCurrentItem;
	}
	
	ImGui::PushID(CallNumber);
	ImGui::Combo("", CurrentItem, values, IM_ARRAYSIZE(values));
	ImGui::PopID();
	return *CurrentItem + 1;
}

Fraction FractionInput(Fraction& InputFraction) {
	// Numerator input
	ImGui::PushID("Numerator");
	ImGui::SetNextItemWidth(50);
	ImGui::InputScalar("", ImGuiDataType_S32, &InputFraction.numerator);
	ImGui::PopID();

	ImGui::SameLine();
	ImGui::Text("/");
	ImGui::SameLine();

	// Denominator input
	ImGui::PushID("Denominator");
	ImGui::SetNextItemWidth(50);
	ImGui::InputScalar("", ImGuiDataType_U32, &InputFraction.denominator);
	InputFraction.denominator = Clamp(InputFraction.denominator, 1, INT32_MAX);
	ImGui::PopID();
	return InputFraction;
}

template<typename VectorType, typename Proc> void InputVector(std::vector<VectorType> &Vector, int Size, Proc PrintFunction) {
	ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + Size * 140, 0.0f));
	ImGui::BeginChild(Vector.size(), ImVec2(0, ImGui::GetFontSize() * 4), false, ImGuiWindowFlags_HorizontalScrollbar);
	
	ImGui::Columns(Size);
	
	// Names of variables
	PrintFunction(Size);
	ImGui::Separator();
	
	for (int i = 0; i < Size; i++) {
		ImGui::PushID(i);
		ImGui::SetNextItemWidth(75);

		// Depeding on type of vector's element we decide how to handle input
		if constexpr (std::is_same<VectorType, float>::value) {
			// Real case
			ImGui::InputScalar("", ImGuiDataType_Float, &Vector.at(i));
		} else {
			// Fractional case
			Vector.at(i) = FractionInput(Vector.at(i));
		}

		if (i < Size - 1) { ImGui::SameLine(); }
		ImGui::PopID();

		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::EndChild();
}

template<typename MatrixType>void MatrixInput(MatrixType& matrix) {
	ImGui::Columns(matrix.ColNumber);
	// Names of variables
	for (int i = 0; i < matrix.ColNumber - 1; i++) {
		ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
		ImGui::NextColumn();
	}
	ImGui::Text("B");
	ImGui::NextColumn();
	ImGui::Separator();

	// Matrix itself
	for (int i = 0; i < matrix.RowNumber - 1; i++) {
		for (int j = 0; j < matrix.ColNumber; j++) {
			ImGui::PushID(i * matrix.ColNumber + j);

			ImGui::SetNextItemWidth(75);
			// Depending on type of matrix we choose two different ways to handle input
			if constexpr (std::is_same<MatrixType, Matrix>::value) {
				// Real case
				ImGui::InputScalar("", ImGuiDataType_Float, &matrix[i][j]);
			} else {
				// Fractional case
				matrix[i][j] = FractionInput(matrix[i][j]);
			}
			if (j < matrix.ColNumber - 1) { ImGui::SameLine(); }

			ImGui::PopID();
			
			ImGui::NextColumn();
		}
		ImGui::Separator();
	}

	ImGui::Columns(1);
}

template<typename MatrixType> void DisplaySolutionVector(MatrixType &matrix, std::vector<int> &NumbersOfVariables, bool IsCompleteSolution) {
	ImGui::Columns(NumbersOfVariables.size());
	for (int i = 0; i < NumbersOfVariables.size(); i++) {
		ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
		ImGui::NextColumn();
	}
	ImGui::Separator();

	// Numbers of base variables
	std::vector<int> BaseVariablesNumbers(NumbersOfVariables.begin(), NumbersOfVariables.begin() + matrix.RowNumber - 1);

	for (int i = 0, RowIndex = 0; i < NumbersOfVariables.size(); i++) {
		int LastColumnIndex = matrix.ColNumber - 1;
		if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
			// Real case
			// If number of variable is base variable
			if (DoesContain(BaseVariablesNumbers, i + 1)) {
				ImGui::Text(std::to_string(matrix[RowIndex][LastColumnIndex]).c_str());
				RowIndex++;
			} else {
				ImGui::Text("%f", 0.0f);
			}
		} else {
			// Fractional case
			if (DoesContain(BaseVariablesNumbers, i + 1)) {
				if (matrix[RowIndex][LastColumnIndex].denominator != 1) {
					// We display denominator if it doesn't equal to 1
					ImGui::Text((std::to_string(matrix[RowIndex][LastColumnIndex].numerator) + std::string("/") + std::to_string(matrix[RowIndex][LastColumnIndex].denominator)).c_str());
				} else {
					// We don't display denominator if it equals to 1
					ImGui::Text((std::to_string(matrix[RowIndex][LastColumnIndex].numerator)).c_str());
				}
				RowIndex++;
			} else {
				ImGui::Text("0");
			}
		}
		ImGui::NextColumn();
	}
	ImGui::Separator();
	ImGui::Columns(1);

	if (IsCompleteSolution) {
		if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
			// Real case
			ImGui::Text(u8"Минимальное значение функции: F(x)= %f", -matrix[matrix.RowNumber - 1][matrix.ColNumber - 1]);
		} else {
			// Fractional case
			if (matrix[matrix.RowNumber - 1][matrix.ColNumber - 1].denominator != 1) {
				ImGui::Text(u8"Минимальное значение функции: F(x)= %d/%d", -matrix[matrix.RowNumber - 1][matrix.ColNumber - 1].numerator, matrix[matrix.RowNumber - 1][matrix.ColNumber - 1].denominator);
			} else {
				ImGui::Text(u8"Минимальное значение функции: F(x)= %d", -matrix[matrix.RowNumber - 1][matrix.ColNumber - 1].numerator);
			}
		}
	}
}

RowAndColumn CurrentLeadPos;
std::vector<RowAndColumn> PotentialLeads;
template<typename MatrixType> void DisplayStepOnScreen(MatrixType& matrix, int StepID, bool IsLastIteration, std::vector<int> NumbersOfVariables) {
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
			
			std::string CellLabel;
			// Labels
			if constexpr (std::is_same<MatrixType, Matrix>::value) {
				// Real case
				CellLabel = std::to_string(matrix[i][j]);
			} else {
				// Fractional case
				if (matrix[i][j].denominator != 1) {
					// We display denominator if it doesn't equal to 1
					CellLabel = (std::to_string(matrix[i][j].numerator) + std::string("/") + std::to_string(matrix[i][j].denominator));
				} else {
					// We don't display denominator if it equals to 1
					CellLabel = (std::to_string(matrix[i][j].numerator));
				}
			}

			// Fill with a color chosen cell
			bool IsThisCellShouldBeButton = false;
			if (state == CONTINUE) {
				float width = ImGui::GetColumnWidth();
				
				for (RowAndColumn ElementRC : PotentialLeads) {
					if (ElementRC.Row == i && ElementRC.Column == j && IsLastIteration) {
						ImGui::PushID(i + j * matrix.ColNumber);
						if ((ElementRC.Row == CurrentLeadPos.Row && ElementRC.Column == CurrentLeadPos.Column)) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
							if (ImGui::Button(CellLabel.c_str(), ImVec2(width - 15.0f, ImGui::GetTextLineHeight() * 1.3f))) {
								CurrentLeadPos = ElementRC;
							}
							ImGui::PopStyleColor();
						} else {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.40f));
							if (ImGui::Button(CellLabel.c_str(), ImVec2(width - 15.0f, ImGui::GetTextLineHeight() * 1.3f))) {
								CurrentLeadPos = ElementRC;
							}
							ImGui::PopStyleColor();
						}
						ImGui::PopID();

						IsThisCellShouldBeButton = true;
					}
				}
			}

			// We already have text for this cell
			if (!IsThisCellShouldBeButton) {
				ImGui::Text(CellLabel.c_str());
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
		for (int i = StartIndex; i < Steps.size(); i++) {
			bool IsLastIteration = (i == Steps.size() - 1);
			if (IsFractionalCoefficients) {
				DisplayStepOnScreen(Steps[i].FracMatrix, Steps[i].StepID, IsLastIteration, Steps[i].NumbersOfVariables);
			} else {
				DisplayStepOnScreen(Steps[i].RealMatrix, Steps[i].StepID, IsLastIteration, Steps[i].NumbersOfVariables);
			}
			ImGui::Separator();
		}
	}
}

}