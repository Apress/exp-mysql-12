#pragma once
#include "DBEngine.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace BookVendingMachine {

  DBEngine  *AdminDatabase = new DBEngine();

	/// <summary>
	/// Summary for Admin
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>

	public ref class AdminForm: public System::Windows::Forms::Form
	{
	public:
		AdminForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~AdminForm()
		{
			if (components)
			{
				delete components;
			}
		}

  protected: 
  private: System::Windows::Forms::TextBox^  txtQuery;
  private: System::Windows::Forms::Button^  btnExecute;
  private: System::Windows::Forms::ListBox^  lstData;
  private: System::Windows::Forms::Label^  label1;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
      this->txtQuery = (gcnew System::Windows::Forms::TextBox());
      this->btnExecute = (gcnew System::Windows::Forms::Button());
      this->lstData = (gcnew System::Windows::Forms::ListBox());
      this->label1 = (gcnew System::Windows::Forms::Label());
      this->SuspendLayout();
      // 
      // txtQuery
      // 
      this->txtQuery->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left) 
        | System::Windows::Forms::AnchorStyles::Right));
      this->txtQuery->Font = (gcnew System::Drawing::Font(L"Courier New", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->txtQuery->Location = System::Drawing::Point(12, 219);
      this->txtQuery->Multiline = true;
      this->txtQuery->Name = L"txtQuery";
      this->txtQuery->Size = System::Drawing::Size(721, 53);
      this->txtQuery->TabIndex = 2;
      // 
      // btnExecute
      // 
      this->btnExecute->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
      this->btnExecute->Location = System::Drawing::Point(658, 278);
      this->btnExecute->Name = L"btnExecute";
      this->btnExecute->Size = System::Drawing::Size(75, 23);
      this->btnExecute->TabIndex = 3;
      this->btnExecute->Text = L"Execute";
      this->btnExecute->UseVisualStyleBackColor = true;
      this->btnExecute->Click += gcnew System::EventHandler(this, &AdminForm::btnExecute_Click);
      // 
      // lstData
      // 
      this->lstData->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
        | System::Windows::Forms::AnchorStyles::Left) 
        | System::Windows::Forms::AnchorStyles::Right));
      this->lstData->Font = (gcnew System::Drawing::Font(L"Courier New", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lstData->FormattingEnabled = true;
      this->lstData->HorizontalScrollbar = true;
      this->lstData->ItemHeight = 16;
      this->lstData->Location = System::Drawing::Point(12, 28);
      this->lstData->MultiColumn = true;
      this->lstData->Name = L"lstData";
      this->lstData->Size = System::Drawing::Size(721, 180);
      this->lstData->TabIndex = 4;
      // 
      // label1
      // 
      this->label1->AutoSize = true;
      this->label1->Font = (gcnew System::Drawing::Font(L"Courier New", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->label1->Location = System::Drawing::Point(12, 9);
      this->label1->Name = L"label1";
      this->label1->Size = System::Drawing::Size(688, 16);
      this->label1->TabIndex = 5;
      this->label1->Text = L"(ISBN, Slot, Quantity, Price, Pages, PubDate, Title, Authors, Thumbnail, Descript" 
        L"ion)";
      // 
      // AdminForm
      // 
      this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
      this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
      this->ClientSize = System::Drawing::Size(745, 311);
      this->Controls->Add(this->label1);
      this->Controls->Add(this->lstData);
      this->Controls->Add(this->btnExecute);
      this->Controls->Add(this->txtQuery);
      this->Name = L"AdminForm";
      this->Text = L"Book Vending Machine Administration";
      this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &AdminForm::AdminForm_FormClosing);
      this->Load += gcnew System::EventHandler(this, &AdminForm::Admin_Load);
      this->ResumeLayout(false);
      this->PerformLayout();

    }
#pragma endregion
  void LoadList() 
  {
    int i = 0;
    int j = 0;
    String^ str;

    lstData->Items->Clear();
    AdminDatabase->StartQuery("SELECT ISBN, Slot, Quantity, Price," \
      " Pages, PubDate, Title, Authors, Thumbnail," \
      " Description FROM books");
    do
    {
      str = gcnew String("");
      for (i = 0; i < 10; i++)
      {
        if (i != 0)
        {
          str = str + "\t";
        }
        str = str + gcnew String(AdminDatabase->GetField(i));
      }
      lstData->Items->Add(str);
      j++;
    }while(AdminDatabase->GetNext());
  }

private: System::Void btnExecute_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    String ^orig = gcnew String(txtQuery->Text->ToString());
    pin_ptr<const wchar_t> wch = PtrToStringChars(orig);

    // Convert to a char*
    size_t origsize = wcslen(wch) + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    char nstring[newsize];
    wcstombs_s(&convertedChars, nstring, origsize, wch, _TRUNCATE);
    AdminDatabase->RunQuery(nstring);
    LoadList();
  }

private: System::Void Admin_Load(System::Object^  sender, System::EventArgs^  e) 
  {
    AdminDatabase->Initialize();
    LoadList();
  }

private: System::Void AdminForm_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) 
  {
    AdminDatabase->Shutdown();
  }
};
}
