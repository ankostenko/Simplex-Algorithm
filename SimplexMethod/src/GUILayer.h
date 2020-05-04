#pragma once

namespace GUILayer {

const char* ComboBoxVariablesLabel = u8"Число переменных ";
const char* ComboBoxLimitationsLabel = u8"Число ограничений";

// TODO: Fix alignment of labels
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

}