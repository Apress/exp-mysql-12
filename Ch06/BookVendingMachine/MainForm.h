#pragma once
#include "DBEngine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "vcclr.h"
#include <time.h> 

namespace BookVendingMachine {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

  const char GREETING[] = "Please make a selection.";

  DBEngine  *Database = new DBEngine();

	/// <summary>
	/// Summary for MainForm
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
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
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}
  private: System::Windows::Forms::Panel^  pnlButtons;
  private: System::Windows::Forms::Button^  btnBook1;
  private: System::Windows::Forms::Button^  btnBook7;
  private: System::Windows::Forms::Button^  btnBook6;
  private: System::Windows::Forms::Button^  btnBook5;
  private: System::Windows::Forms::Button^  btnBook4;
  private: System::Windows::Forms::Button^  btnBook3;
  private: System::Windows::Forms::Button^  btnBook2;
  private: System::Windows::Forms::Button^  btnBook10;
  private: System::Windows::Forms::Button^  btnBook9;
  private: System::Windows::Forms::Button^  btnBook8;
  private: System::Windows::Forms::Panel^  pnlDetail;
  private: System::Windows::Forms::RichTextBox^  txtDescription;
  private: System::Windows::Forms::Label^  lblPubDateCap;
  private: System::Windows::Forms::Label^  lblNumPages;
  private: System::Windows::Forms::Label^  lblPagesCap;
  private: System::Windows::Forms::Label^  lblPrice;
  private: System::Windows::Forms::Label^  lblPriceCap;
  private: System::Windows::Forms::Label^  lblAuthors;
  private: System::Windows::Forms::Label^  lblTitle;
  private: System::Windows::Forms::Label^  lblPubDate;
  private: System::Windows::Forms::Label^  lblISBN;
  private: System::Windows::Forms::Button^  btnCancel;
  private: System::Windows::Forms::Button^  btnPurchase;

  private: System::Windows::Forms::Label^  lblStatus;
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
      System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MainForm::typeid));
      this->pnlButtons = (gcnew System::Windows::Forms::Panel());
      this->btnBook10 = (gcnew System::Windows::Forms::Button());
      this->btnBook9 = (gcnew System::Windows::Forms::Button());
      this->btnBook8 = (gcnew System::Windows::Forms::Button());
      this->btnBook7 = (gcnew System::Windows::Forms::Button());
      this->btnBook6 = (gcnew System::Windows::Forms::Button());
      this->btnBook5 = (gcnew System::Windows::Forms::Button());
      this->btnBook4 = (gcnew System::Windows::Forms::Button());
      this->btnBook3 = (gcnew System::Windows::Forms::Button());
      this->btnBook2 = (gcnew System::Windows::Forms::Button());
      this->btnBook1 = (gcnew System::Windows::Forms::Button());
      this->pnlDetail = (gcnew System::Windows::Forms::Panel());
      this->btnCancel = (gcnew System::Windows::Forms::Button());
      this->btnPurchase = (gcnew System::Windows::Forms::Button());
      this->lblISBN = (gcnew System::Windows::Forms::Label());
      this->lblPubDate = (gcnew System::Windows::Forms::Label());
      this->lblPubDateCap = (gcnew System::Windows::Forms::Label());
      this->lblNumPages = (gcnew System::Windows::Forms::Label());
      this->lblPagesCap = (gcnew System::Windows::Forms::Label());
      this->lblPrice = (gcnew System::Windows::Forms::Label());
      this->lblPriceCap = (gcnew System::Windows::Forms::Label());
      this->lblAuthors = (gcnew System::Windows::Forms::Label());
      this->lblTitle = (gcnew System::Windows::Forms::Label());
      this->txtDescription = (gcnew System::Windows::Forms::RichTextBox());
      this->lblStatus = (gcnew System::Windows::Forms::Label());
      this->pnlButtons->SuspendLayout();
      this->pnlDetail->SuspendLayout();
      this->SuspendLayout();
      // 
      // pnlButtons
      // 
      this->pnlButtons->Controls->Add(this->btnBook10);
      this->pnlButtons->Controls->Add(this->btnBook9);
      this->pnlButtons->Controls->Add(this->btnBook8);
      this->pnlButtons->Controls->Add(this->btnBook7);
      this->pnlButtons->Controls->Add(this->btnBook6);
      this->pnlButtons->Controls->Add(this->btnBook5);
      this->pnlButtons->Controls->Add(this->btnBook4);
      this->pnlButtons->Controls->Add(this->btnBook3);
      this->pnlButtons->Controls->Add(this->btnBook2);
      this->pnlButtons->Controls->Add(this->btnBook1);
      this->pnlButtons->Location = System::Drawing::Point(9, 6);
      this->pnlButtons->Name = L"pnlButtons";
      this->pnlButtons->Size = System::Drawing::Size(628, 327);
      this->pnlButtons->TabIndex = 0;
      // 
      // btnBook10
      // 
      this->btnBook10->Location = System::Drawing::Point(499, 160);
      this->btnBook10->Name = L"btnBook10";
      this->btnBook10->Size = System::Drawing::Size(129, 164);
      this->btnBook10->TabIndex = 9;
      this->btnBook10->UseVisualStyleBackColor = true;
      this->btnBook10->Click += gcnew System::EventHandler(this, &MainForm::btnBook10_Click);
      // 
      // btnBook9
      // 
      this->btnBook9->Location = System::Drawing::Point(375, 161);
      this->btnBook9->Name = L"btnBook9";
      this->btnBook9->Size = System::Drawing::Size(129, 164);
      this->btnBook9->TabIndex = 8;
      this->btnBook9->UseVisualStyleBackColor = true;
      this->btnBook9->Click += gcnew System::EventHandler(this, &MainForm::btnBook9_Click);
      // 
      // btnBook8
      // 
      this->btnBook8->Location = System::Drawing::Point(249, 161);
      this->btnBook8->Name = L"btnBook8";
      this->btnBook8->Size = System::Drawing::Size(129, 164);
      this->btnBook8->TabIndex = 7;
      this->btnBook8->UseVisualStyleBackColor = true;
      this->btnBook8->Click += gcnew System::EventHandler(this, &MainForm::btnBook8_Click);
      // 
      // btnBook7
      // 
      this->btnBook7->Location = System::Drawing::Point(124, 161);
      this->btnBook7->Name = L"btnBook7";
      this->btnBook7->Size = System::Drawing::Size(129, 164);
      this->btnBook7->TabIndex = 6;
      this->btnBook7->UseVisualStyleBackColor = true;
      this->btnBook7->Click += gcnew System::EventHandler(this, &MainForm::btnBook7_Click);
      // 
      // btnBook6
      // 
      this->btnBook6->Location = System::Drawing::Point(0, 161);
      this->btnBook6->Name = L"btnBook6";
      this->btnBook6->Size = System::Drawing::Size(129, 164);
      this->btnBook6->TabIndex = 5;
      this->btnBook6->UseVisualStyleBackColor = true;
      this->btnBook6->Click += gcnew System::EventHandler(this, &MainForm::btnBook6_Click);
      // 
      // btnBook5
      // 
      this->btnBook5->Location = System::Drawing::Point(498, 0);
      this->btnBook5->Name = L"btnBook5";
      this->btnBook5->Size = System::Drawing::Size(129, 164);
      this->btnBook5->TabIndex = 4;
      this->btnBook5->UseVisualStyleBackColor = true;
      this->btnBook5->Click += gcnew System::EventHandler(this, &MainForm::btnBook5_Click);
      // 
      // btnBook4
      // 
      this->btnBook4->Location = System::Drawing::Point(375, 0);
      this->btnBook4->Name = L"btnBook4";
      this->btnBook4->Size = System::Drawing::Size(129, 164);
      this->btnBook4->TabIndex = 3;
      this->btnBook4->UseVisualStyleBackColor = true;
      this->btnBook4->Click += gcnew System::EventHandler(this, &MainForm::btnBook4_Click);
      // 
      // btnBook3
      // 
      this->btnBook3->Location = System::Drawing::Point(249, 0);
      this->btnBook3->Name = L"btnBook3";
      this->btnBook3->Size = System::Drawing::Size(129, 164);
      this->btnBook3->TabIndex = 2;
      this->btnBook3->UseVisualStyleBackColor = true;
      this->btnBook3->Click += gcnew System::EventHandler(this, &MainForm::btnBook3_Click);
      // 
      // btnBook2
      // 
      this->btnBook2->Location = System::Drawing::Point(124, 0);
      this->btnBook2->Name = L"btnBook2";
      this->btnBook2->Size = System::Drawing::Size(129, 164);
      this->btnBook2->TabIndex = 1;
      this->btnBook2->UseVisualStyleBackColor = true;
      this->btnBook2->Click += gcnew System::EventHandler(this, &MainForm::btnBook2_Click);
      // 
      // btnBook1
      // 
      this->btnBook1->Location = System::Drawing::Point(0, 0);
      this->btnBook1->Name = L"btnBook1";
      this->btnBook1->Size = System::Drawing::Size(129, 164);
      this->btnBook1->TabIndex = 0;
      this->btnBook1->UseVisualStyleBackColor = true;
      this->btnBook1->Click += gcnew System::EventHandler(this, &MainForm::btnBook1_Click);
      // 
      // pnlDetail
      // 
      this->pnlDetail->Controls->Add(this->btnCancel);
      this->pnlDetail->Controls->Add(this->btnPurchase);
      this->pnlDetail->Controls->Add(this->lblISBN);
      this->pnlDetail->Controls->Add(this->lblPubDate);
      this->pnlDetail->Controls->Add(this->lblPubDateCap);
      this->pnlDetail->Controls->Add(this->lblNumPages);
      this->pnlDetail->Controls->Add(this->lblPagesCap);
      this->pnlDetail->Controls->Add(this->lblPrice);
      this->pnlDetail->Controls->Add(this->lblPriceCap);
      this->pnlDetail->Controls->Add(this->lblAuthors);
      this->pnlDetail->Controls->Add(this->lblTitle);
      this->pnlDetail->Controls->Add(this->txtDescription);
      this->pnlDetail->Location = System::Drawing::Point(7, 6);
      this->pnlDetail->Name = L"pnlDetail";
      this->pnlDetail->Size = System::Drawing::Size(635, 416);
      this->pnlDetail->TabIndex = 2;
      this->pnlDetail->Visible = false;
      // 
      // btnCancel
      // 
      this->btnCancel->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->btnCancel->Location = System::Drawing::Point(452, 374);
      this->btnCancel->Name = L"btnCancel";
      this->btnCancel->Size = System::Drawing::Size(79, 35);
      this->btnCancel->TabIndex = 11;
      this->btnCancel->Text = L"Cancel";
      this->btnCancel->UseVisualStyleBackColor = true;
      this->btnCancel->Click += gcnew System::EventHandler(this, &MainForm::btnCancel_Click);
      // 
      // btnPurchase
      // 
      this->btnPurchase->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->btnPurchase->Location = System::Drawing::Point(537, 374);
      this->btnPurchase->Name = L"btnPurchase";
      this->btnPurchase->Size = System::Drawing::Size(79, 35);
      this->btnPurchase->TabIndex = 10;
      this->btnPurchase->Text = L"Purchase";
      this->btnPurchase->UseVisualStyleBackColor = true;
      this->btnPurchase->Click += gcnew System::EventHandler(this, &MainForm::btnPurchase_Click);
      // 
      // lblISBN
      // 
      this->lblISBN->AutoSize = true;
      this->lblISBN->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblISBN->Location = System::Drawing::Point(18, 59);
      this->lblISBN->Name = L"lblISBN";
      this->lblISBN->Size = System::Drawing::Size(51, 20);
      this->lblISBN->TabIndex = 9;
      this->lblISBN->Text = L"label1";
      // 
      // lblPubDate
      // 
      this->lblPubDate->AutoSize = true;
      this->lblPubDate->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblPubDate->Location = System::Drawing::Point(497, 345);
      this->lblPubDate->Name = L"lblPubDate";
      this->lblPubDate->Size = System::Drawing::Size(51, 20);
      this->lblPubDate->TabIndex = 8;
      this->lblPubDate->Text = L"label1";
      // 
      // lblPubDateCap
      // 
      this->lblPubDateCap->AutoSize = true;
      this->lblPubDateCap->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblPubDateCap->Location = System::Drawing::Point(411, 345);
      this->lblPubDateCap->Name = L"lblPubDateCap";
      this->lblPubDateCap->Size = System::Drawing::Size(89, 20);
      this->lblPubDateCap->TabIndex = 7;
      this->lblPubDateCap->Text = L"Pub Date:";
      // 
      // lblNumPages
      // 
      this->lblNumPages->AutoSize = true;
      this->lblNumPages->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblNumPages->Location = System::Drawing::Point(289, 345);
      this->lblNumPages->Name = L"lblNumPages";
      this->lblNumPages->Size = System::Drawing::Size(51, 20);
      this->lblNumPages->TabIndex = 6;
      this->lblNumPages->Text = L"label1";
      // 
      // lblPagesCap
      // 
      this->lblPagesCap->AutoSize = true;
      this->lblPagesCap->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblPagesCap->Location = System::Drawing::Point(184, 345);
      this->lblPagesCap->Name = L"lblPagesCap";
      this->lblPagesCap->Size = System::Drawing::Size(110, 20);
      this->lblPagesCap->TabIndex = 5;
      this->lblPagesCap->Text = L"Num Pages: ";
      // 
      // lblPrice
      // 
      this->lblPrice->AutoSize = true;
      this->lblPrice->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblPrice->Location = System::Drawing::Point(64, 345);
      this->lblPrice->Name = L"lblPrice";
      this->lblPrice->Size = System::Drawing::Size(51, 20);
      this->lblPrice->TabIndex = 4;
      this->lblPrice->Text = L"label1";
      // 
      // lblPriceCap
      // 
      this->lblPriceCap->AutoSize = true;
      this->lblPriceCap->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblPriceCap->Location = System::Drawing::Point(18, 345);
      this->lblPriceCap->Name = L"lblPriceCap";
      this->lblPriceCap->Size = System::Drawing::Size(54, 20);
      this->lblPriceCap->TabIndex = 3;
      this->lblPriceCap->Text = L"Price:";
      // 
      // lblAuthors
      // 
      this->lblAuthors->AutoSize = true;
      this->lblAuthors->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblAuthors->Location = System::Drawing::Point(18, 36);
      this->lblAuthors->Name = L"lblAuthors";
      this->lblAuthors->Size = System::Drawing::Size(51, 20);
      this->lblAuthors->TabIndex = 2;
      this->lblAuthors->Text = L"label1";
      // 
      // lblTitle
      // 
      this->lblTitle->AutoSize = true;
      this->lblTitle->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblTitle->Location = System::Drawing::Point(18, 12);
      this->lblTitle->Name = L"lblTitle";
      this->lblTitle->Size = System::Drawing::Size(57, 20);
      this->lblTitle->TabIndex = 1;
      this->lblTitle->Text = L"label1";
      // 
      // txtDescription
      // 
      this->txtDescription->BackColor = System::Drawing::SystemColors::MenuBar;
      this->txtDescription->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->txtDescription->Location = System::Drawing::Point(21, 84);
      this->txtDescription->Name = L"txtDescription";
      this->txtDescription->Size = System::Drawing::Size(595, 242);
      this->txtDescription->TabIndex = 0;
      this->txtDescription->Text = L"";
      // 
      // lblStatus
      // 
      this->lblStatus->AutoSize = true;
      this->lblStatus->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
        static_cast<System::Byte>(0)));
      this->lblStatus->Location = System::Drawing::Point(25, 360);
      this->lblStatus->Name = L"lblStatus";
      this->lblStatus->Size = System::Drawing::Size(184, 20);
      this->lblStatus->TabIndex = 3;
      this->lblStatus->Text = L"Please make a selection.";
      // 
      // MainForm
      // 
      this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
      this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
      this->ClientSize = System::Drawing::Size(647, 426);
      this->Controls->Add(this->lblStatus);
      this->Controls->Add(this->pnlDetail);
      this->Controls->Add(this->pnlButtons);
      this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Fixed3D;
      this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
      this->MaximizeBox = false;
      this->MinimizeBox = false;
      this->Name = L"MainForm";
      this->Text = L"Book Vending Machine";
      this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MainForm::MainForm_FormClosing);
      this->Load += gcnew System::EventHandler(this, &MainForm::MainForm_Load);
      this->pnlButtons->ResumeLayout(false);
      this->pnlDetail->ResumeLayout(false);
      this->pnlDetail->PerformLayout();
      this->ResumeLayout(false);
      this->PerformLayout();

    }
#pragma endregion
  void DisplayError()
  {
    String ^str = gcnew String("There was an error with the database system.\n" \
                               "Please contact product support.\nError = ");
    str = str + gcnew String(Database->GetError());
    MessageBox::Show(str, "Internal System Error", MessageBoxButtons::OK, 
                     MessageBoxIcon::Information);
  }

  void LoadDetails(int Slot) 
  {
    int Qty = Database->GetBookFieldInt(Slot, "Quantity");
    if (Database->Error()) DisplayError();
    pnlButtons->Visible = false;
    pnlDetail->Visible = true;
    lblStatus->Visible = false;
    lblTitle->Text = gcnew String(Database->GetBookFieldStr(Slot, "Title"));
    if (Database->Error()) DisplayError();
    lblAuthors->Text = gcnew String(Database->GetBookFieldStr(Slot, "Authors"));
    if (Database->Error()) DisplayError();
    lblISBN->Text = gcnew String(Database->GetBookFieldStr(Slot, "ISBN"));
    if (Database->Error()) DisplayError();
    txtDescription->Text = gcnew String(Database->GetBookFieldText(Slot, "Description"));
    if (Database->Error()) DisplayError();
    lblPrice->Text = gcnew String(Database->GetBookFieldStr(Slot, "Price"));
    if (Database->Error()) DisplayError();
    lblNumPages->Text = gcnew String(Database->GetBookFieldStr(Slot, "Pages"));
    if (Database->Error()) DisplayError();
    lblPubDate->Text = gcnew String(Database->GetBookFieldStr(Slot, "PubDate"));
    if (Database->Error()) DisplayError();
    if(Qty < 1)
    {
      btnPurchase->Enabled = false;
    }
  }

  void CheckAvailability()
  {
    btnBook1->Enabled = (Database->GetBookFieldInt(1, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook2->Enabled = (Database->GetBookFieldInt(2, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook3->Enabled = (Database->GetBookFieldInt(3, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook4->Enabled = (Database->GetBookFieldInt(4, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook5->Enabled = (Database->GetBookFieldInt(5, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook6->Enabled = (Database->GetBookFieldInt(6, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook7->Enabled = (Database->GetBookFieldInt(7, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook8->Enabled = (Database->GetBookFieldInt(8, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook9->Enabled = (Database->GetBookFieldInt(9, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
    btnBook10->Enabled = (Database->GetBookFieldInt(10, "Quantity") >= 1);
    if (Database->Error()) DisplayError();
  }

  void Delay(int secs)
  {
    time_t start;
    time_t current;

    time(&start);
    do
    {
      time(&current);
    } while(difftime(current,start) < secs);
  }

private: System::Void btnCancel_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    lblStatus->Visible = true;
    pnlDetail->Visible = false;
    pnlButtons->Visible = true;
    btnPurchase->Enabled = true;
    lblStatus->Text = gcnew String(GREETING);
  }

private: System::Void btnPurchase_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    String ^orig = gcnew String(lblISBN->Text->ToString());
    pin_ptr<const wchar_t> wch = PtrToStringChars(orig);

    // Convert to a char*
    size_t origsize = wcslen(wch) + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    char nstring[newsize];
    wcstombs_s(&convertedChars, nstring, origsize, wch, _TRUNCATE);

    lblStatus->Visible = true;
    pnlDetail->Visible = false;
    pnlButtons->Visible = true;
    btnPurchase->Enabled = true;
    Database->VendBook(nstring);
    //
    // Simulate buying the book.
    //
    lblStatus->Text = "Please Insert your credit card.";
    this->Refresh();
    //Delay(3); 
    lblStatus->Text = "Thank you. Processing card number ending in 4-1234.";
    this->Refresh();
    //Delay(3);
    lblStatus->Text = "Vending....";
    this->Refresh();
    //Delay(5);
    this->Refresh();
    CheckAvailability();
    lblStatus->Text = gcnew String(GREETING);
  }

private: System::Void MainForm_Load(System::Object^  sender, System::EventArgs^  e) 
  {
    String ^imageName;
    String ^imagePath;

    Database->Initialize();
    if (Database->Error()) DisplayError();
    //
    //For each button, check to see if there are sufficient qty and load
    //the thumbnail for each.
    //
    imagePath = gcnew String(Database->GetSetting("ImagePath"));

    imageName = imagePath + gcnew String(Database->GetBookFieldStr(1, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook1->Image = btnBook1->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(2, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook2->Image = btnBook2->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(3, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook3->Image = btnBook3->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(4, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook4->Image = btnBook4->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(5, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook5->Image = btnBook5->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(6, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook6->Image = btnBook6->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(7, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook7->Image = btnBook7->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(8, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook8->Image = btnBook8->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(9, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook9->Image = btnBook9->Image->FromFile(imageName);
    imageName = imagePath + gcnew String(Database->GetBookFieldStr(10, "Thumbnail"));
    if (Database->Error()) DisplayError();
    btnBook10->Image = btnBook10->Image->FromFile(imageName);

    CheckAvailability();
  }

private: System::Void btnBook1_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(1);
  }

private: System::Void btnBook2_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(2);
  }

private: System::Void btnBook3_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(3);
  }

private: System::Void btnBook4_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(4);
  }

private: System::Void btnBook5_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(5);
  }

private: System::Void btnBook6_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(6);
  }

private: System::Void btnBook7_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(7);
  }

private: System::Void btnBook8_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(8);
  }

private: System::Void btnBook9_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(9);
  }

private: System::Void btnBook10_Click(System::Object^  sender, System::EventArgs^  e) 
  {
    LoadDetails(10);
  }

private: System::Void MainForm_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) 
  {
    Database->Shutdown();
  }

};
}

