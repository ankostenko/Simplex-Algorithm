#pragma once

namespace GUILayer {

const char* ComboBoxVariablesLabel = u8"Число переменных ";
const char* ComboBoxLimitationsLabel = u8"Число ограничений";

static void HelpMarker(const char* desc) {
	ImGui::PushID(desc);
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	ImGui::PopID();
}

int ComboBox(const char *label, int FileVariables, int FileLimitations, bool FileReadHasHappened) {
	const char* values[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" };
	static int CallNumber = 0;
	CallNumber = CallNumber == 0 ? 1 : 0;

	// Align labels of comboboxes
	ImGui::Text(label); ImGui::SameLine();
	ImGui::SetNextItemWidth(75);

	static int CallOneCurrentItem = 0;
	static int CallTwoCurrentItem = 0;

	if (CallTwoCurrentItem == 0) { CallTwoCurrentItem = 1; }
	if (CallOneCurrentItem >= CallTwoCurrentItem) { CallOneCurrentItem = CallTwoCurrentItem - 1; }

	static int *CurrentItem = NULL;
	if (CallNumber == 0) {
		if (FileReadHasHappened) { CallOneCurrentItem = FileLimitations; }
		CurrentItem = &CallOneCurrentItem;
	} else {
		if (FileReadHasHappened) { CallTwoCurrentItem = FileVariables; }
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

template<typename VectorType, typename Proc> void InputVector(std::vector<VectorType> &Vector, std::vector<bool> *BasisActive, int Size, Proc PrintFunction) {
	ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetCursorPos().x + Size * 170.0f, 0.0f));
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
			ImGui::InputScalar("", ImGuiDataType_Float, &Vector[i]);
		} else {
			// Fractional case
			Vector.at(i) = FractionInput(Vector[i]);
		}

		if (BasisActive) {
			bool temp = (*BasisActive)[i];
			ImGui::PushID((std::string("check") + std::to_string(i)).c_str());
			ImGui::SameLine(); ImGui::Checkbox("", &temp);
			ImGui::PopID();
			(*BasisActive)[i] = temp;
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

template<typename MatrixType> void DisplaySolutionVector(MatrixType &matrix, std::vector<int> &BaseVariables, int TotalSize, bool IsCompleteSolution) {
	ImGui::Columns(TotalSize);
	for (int i = 0; i < TotalSize; i++) {
		ImGui::Text((std::string("x") + std::to_string(i + 1)).c_str());
		ImGui::NextColumn();
	}
	ImGui::Separator();

	// Display resulting vector
	int LastColumnIndex = matrix.ColNumber - 1;
	for (int i = 0, BVCounter = 0; i < TotalSize; i++) {
		if constexpr (IS_SAME_TYPE(MatrixType, Matrix)) {
			if (BVCounter < BaseVariables.size() && i + 1 == BaseVariables[BVCounter]) {
				ImGui::Text(std::to_string(matrix[BVCounter][LastColumnIndex]).c_str());
				BVCounter += 1;
			} else {
				ImGui::Text(std::to_string(0.0f).c_str());
			}
		} else {
			if (BVCounter < BaseVariables.size() && i + 1 == BaseVariables[BVCounter]) {
				if (matrix[BVCounter][LastColumnIndex].denominator != 1) {
					// We display denominator if it doesn't equal to 1
					ImGui::Text((std::to_string(matrix[BVCounter][LastColumnIndex].numerator) + std::string("/") + std::to_string(matrix[BVCounter][LastColumnIndex].denominator)).c_str());
				} else {
					// We don't display denominator if it equals to 1
					ImGui::Text((std::to_string(matrix[BVCounter][LastColumnIndex].numerator)).c_str());
				}
				BVCounter += 1;
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

	// Display free variables
	// Only suitable for simplex algorithm's steps
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
						ImGui::PushID(j + i * matrix.ColNumber);
						if ((ElementRC.Row == CurrentLeadPos.Row && ElementRC.Column == CurrentLeadPos.Column)) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4((float)249 / 255, (float)105 / 255, (float)14 / 255, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4((float)249 / 255, (float)105 / 255, (float)14 / 255, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4((float)211 / 255, (float)84 / 255, (float)0 / 255, 1.0f));
							if (ImGui::Button(CellLabel.c_str(), ImVec2(width - 15.0f, ImGui::GetTextLineHeight() * 1.3f))) {
								CurrentLeadPos = ElementRC;
							}
							ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();
						} else {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4((float)249 / 255, (float)191 / 255, (float)59 / 255, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4((float)249 / 255, (float)105 / 255, (float)14 / 255, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4((float)211 / 255, (float)84 / 255, (float)0 / 255, 1.0f));
							if (ImGui::Button(CellLabel.c_str(), ImVec2(width - 15.0f, ImGui::GetTextLineHeight() * 1.3f))) {
								CurrentLeadPos = ElementRC;
							}
							ImGui::PopStyleColor(); ImGui::PopStyleColor(); ImGui::PopStyleColor();
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

bool MessageWindow(const char *Message) {
	ImGui::OpenPopup(u8"Ошибка");

	if (ImGui::BeginPopupModal(u8"Ошибка", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(Message).x / 2);
		ImGui::Text(Message);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30.0f);
		if (ImGui::Button(u8"ОК", ImVec2(60, 30))) { 
			ImGui::CloseCurrentPopup(); 
			ImGui::EndPopup();
			return true;
		}
		ImGui::EndPopup();
	}

	return false;
}

int ErrorMessage(const char *ErrorMessage) {
	ImGui::OpenPopup(u8"Ошибка");

	ImGui::SetNextWindowSize(ImVec2(550, 150), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal(u8"Ошибка", 0)) {
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(ErrorMessage).x / 2);
		ImGui::TextWrapped(ErrorMessage);
		ImGui::BeginChild("Error", ImVec2(0, 40), true);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(u8"Использовать метод искусственного базиса?").x / 2);
		ImGui::TextWrapped(u8"Использовать метод искусственного базиса?");
		ImGui::EndChild();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60.0f);
		if (ImGui::Button(u8"Да", ImVec2(60, 30))) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return 0;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"Нет", ImVec2(60, 30))) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return 1;
			
		}
		ImGui::EndPopup();
	}
	return 2;
}

void ReferencePopup(bool &OpenReferencePopup) {
	ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal(u8"Справка", &OpenReferencePopup)) {
		if (ImGui::BeginTabBar("Reference")) {
			if (ImGui::BeginTabItem(u8"Как пользоваться программой")) {
				ImGui::BeginChild("User guide", ImVec2(0, 330), true);
				ImGui::TextWrapped(u8"Начало работы:");
				ImGui::TextWrapped(u8"1. Введите размерности.\n2. Выберите параметры решения задачи.\n3. Введите данные.\n4. Нажмите решить.");
				ImGui::Separator();
				ImGui::TextWrapped(u8"После метода искусственного базиса и явно заданного базиса внизу появится кнопка "
									"'Продолжить симплекс алгоритм', возможно, нужно будет пролистнуть немного вниз, чтобы ее увидеть");
				ImGui::TextWrapped(u8"После завершения решения ответ будет показываться внизу экрана.");
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(u8"Работа с файлами")) {
				if (ImGui::BeginTabBar("Files")) {
					if (ImGui::BeginTabItem(u8"Чтение")) {
						ImGui::BeginChild("File Work", ImVec2(0, 165), true);
						ImGui::TextWrapped(u8"Два первых числа - размерность задачи.\nПервое число - количество ограничений, второе - количество столбцов в матрице.");
						ImGui::TextWrapped(u8"Затем по порядку написаны все элементы матрицы.\nВ случае дробей до и после '/' не должно быть пробелов. Знаменатель должен быть явно указан.");
						ImGui::EndChild();
						ImGui::TextWrapped(u8"Пример:");
						ImGui::Text(u8"Действительные числа");
						ImGui::BeginChild("Example", ImVec2(0, 215), true);
						ImGui::Text(u8"2\n3\n1 2 3\n1 2 3");
						ImGui::Text(u8"Обыкновенные дроби");
						ImGui::Text(u8"2\n3\n1/1 2/1 3/1\n1/1 2/1 3/1");
						ImGui::EndChild();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem(u8"Запись")) {
						ImGui::BeginChild("Writing", ImVec2(0, 130), true);
						ImGui::TextWrapped(u8"При сохранении будет сохраняться выбранная матрица,"
							" а не та, которая в данный момент показана на экране, поэтому," 
							" если была введена только матрица с действительными коэффициентами, а сохраняется как с обыкновенными дробями,"
							" то будет сохранена матрица с обыкновенными дробями, но с неинициализированными элементами.\n");
						ImGui::EndChild();
						ImGui::TextWrapped(u8"При сохранении в имени файла нужно явно указывать расширение\nПример:");
						ImGui::BeginChild("Txt Example", ImVec2(0, 70), true);
						ImGui::TextWrapped(u8"Файл.txt");
						ImGui::EndChild();
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	
		if (ImGui::Button(u8"Закрыть")) {
			ImGui::CloseCurrentPopup();
			OpenReferencePopup = false;
		}

		ImGui::EndPopup();
	}
}

void MainMenuBar(Matrix& RealMatrix, FractionalMatrix& FracMatrix, std::vector<bool> &BasisActive, int& OutNumberOfVariables, int& OutNumberOfLimitations, bool& IsReadHasHappened, int& IsFractionalCoeffs) {
	// Default path initialization
	static WCHAR DEFAULT_PATH[256];
	GetModuleFileName(NULL, (WCHAR*)DEFAULT_PATH, 256);
	static std::regex target(u8"SimplexMethod.exe");
	static std::wstring STR_DEFAULT_PATH(DEFAULT_PATH);
	static std::string StrDEFULAT_PATH = std::regex_replace(std::string(STR_DEFAULT_PATH.begin(), STR_DEFAULT_PATH.end()), target, " ");
	// Patterns
	char const* lFilterPatterns[2] = { "*.txt", "*.text" };


	static bool OpenAboutPopup = false;
	static bool OpenFileOpenPopup = false;
	static bool OpenFileSavePopup = false;
	static bool OpenReferencePopup = false;
	static int IsFractionalCoefficients = 0;
	static bool ErrorOccured = false;
	const static char* ErrorMessage;

	if (ErrorOccured) {
		// If window is closed
		if (MessageWindow(ErrorMessage)) {
			ErrorOccured = false;
		}
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu(u8"Файл")) {
			if (ImGui::MenuItem(u8"Открыть...")) {
				OpenFileOpenPopup = true;
			}
			if (ImGui::MenuItem(u8"Сохранить")) {
				OpenFileSavePopup = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"Справка")) {
			if (ImGui::MenuItem(u8"Справка")) {
				OpenReferencePopup = true;
			}
			if (ImGui::MenuItem(u8"О программе")) {
				OpenAboutPopup = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (OpenAboutPopup) { ImGui::OpenPopup(u8"О программе"); }
	if (OpenFileOpenPopup) { ImGui::OpenPopup(u8"Открыть файл"); }
	if (OpenFileSavePopup) { ImGui::OpenPopup(u8"Сохранить файл"); }
	if (OpenReferencePopup) { ImGui::OpenPopup(u8"Справка"); }

	// Reference popup
	ReferencePopup(OpenReferencePopup);

	ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowSize().x * 0.7f, 160.0f));
	if (ImGui::BeginPopupModal(u8"О программе", &OpenAboutPopup, ImGuiWindowFlags_NoResize)) {
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

	ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(240, 240, 240, 255));
	if (ImGui::BeginPopupModal(u8"Открыть файл", &OpenFileOpenPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::BeginChild("OpenFile", ImVec2(400, 70), true);
		ImGui::Text(u8"Выберите тип элементов");
		ImGui::Separator();
		ImGui::PushID(1);
		ImGui::Combo("", &IsFractionalCoefficients, u8"Действительные числа\0Обыкновенные дроби\0");
		ImGui::PopID();

		ImGui::EndChild();
		if (ImGui::Button(u8"Применить")) {
			const char* path = tinyfd_openFileDialog("Открыть файл", StrDEFULAT_PATH.c_str(), 2, lFilterPatterns, NULL, false);
			std::string filename;
			if (path != NULL) {
				filename = path;
			}

			if (!filename.empty()) {
				IsReadHasHappened = true;
				IsFractionalCoeffs = IsFractionalCoefficients;

				FILE* file = fopen(filename.c_str(), "r");
				// If it asserts something went horribly wrong
				assert(file);

				int NumberOfLimitations = -1;
				int NumberOfVariables = -1;
				if (fscanf(file, "%d", &NumberOfLimitations) != 1 || fscanf(file, "%d", &NumberOfVariables) != 1) {
					IsReadHasHappened = false;
					ErrorOccured = true;
					ErrorMessage = u8"Произошла ошибка при чтении размерности задачи!";
				}

				if (NumberOfVariables < 1 || NumberOfLimitations < 1 || NumberOfLimitations >= NumberOfVariables) {
					IsReadHasHappened = false;
					ErrorOccured = true;
					ErrorMessage = u8"Неверная размерность";
				}

				if (!ErrorOccured) {
					OutNumberOfLimitations = NumberOfLimitations;
					OutNumberOfVariables = NumberOfVariables;
					FracMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables);
					RealMatrix.Resize(NumberOfLimitations + 1, NumberOfVariables);
					BasisActive.resize(NumberOfVariables - 1);

					if (IsFractionalCoefficients) {
						for (int i = 0; i < NumberOfLimitations; i++) {
							for (int j = 0; j < NumberOfVariables; j++) {
								if (fscanf(file, "%d/%d", &FracMatrix[i][j].numerator, &FracMatrix[i][j].denominator) != 2) {
									IsReadHasHappened = false;
									ErrorOccured = true;
									ErrorMessage = u8"Произошла ошибка при чтении матрицы ограничений!";
								}
							}
							if (ErrorOccured) { break; }
						}
					} else {
						for (int i = 0; i < NumberOfLimitations; i++) {
							for (int j = 0; j < NumberOfVariables; j++) {
								if (fscanf(file, "%f", &RealMatrix[i][j]) != 1) {
									IsReadHasHappened = false;
									ErrorOccured = true;
									ErrorMessage = u8"Произошла ошибка при чтении матрицы ограничений!";
								}
							}
							if (ErrorOccured) { break; }
						}
					}
				}

				fclose(file);
			}

			OpenFileOpenPopup = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(240, 240, 240, 255));
	if (ImGui::BeginPopupModal(u8"Сохранить файл", &OpenFileSavePopup, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::BeginChild("SaveFile", ImVec2(400, 70), true);
		ImGui::Text(u8"Выберите тип элементов");
		ImGui::Separator();
		ImGui::PushID(1);
		ImGui::Combo("", &IsFractionalCoefficients, u8"Действительные числа\0Обыкновенные дроби\0");
		ImGui::PopID();
		ImGui::EndChild();

		if (ImGui::Button(u8"Применить")) {
			const char* path = tinyfd_saveFileDialog(u8"Открыть файл", StrDEFULAT_PATH.c_str(), 2, lFilterPatterns, NULL);
			std::string filename;
			if (path != NULL) {
				filename = path;
			}

			if (!filename.empty()) {
				FILE* file = fopen(filename.c_str(), "w");
				assert(file != NULL);

				if (IsFractionalCoefficients) {
					fprintf(file, "%d\n%d\n", FracMatrix.RowNumber - 1, FracMatrix.ColNumber);
					for (int i = 0; i < FracMatrix.RowNumber - 1; i++) {
						for (int j = 0; j < FracMatrix.ColNumber; j++) {
							fprintf(file, "%d/%d ", FracMatrix[i][j].numerator, FracMatrix[i][j].denominator);
						}
						fprintf(file, "\n");
					}
				} else {
					fprintf(file, "%d\n%d\n", RealMatrix.RowNumber - 1, RealMatrix.ColNumber);
					for (int i = 0; i < RealMatrix.RowNumber - 1; i++) {
						for (int j = 0; j < RealMatrix.ColNumber; j++) {
							fprintf(file, "%f ", RealMatrix[i][j]);
						}
						fprintf(file, "\n");
					}
				}

				fclose(file);
			}
			OpenFileSavePopup = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleColor();
}

}