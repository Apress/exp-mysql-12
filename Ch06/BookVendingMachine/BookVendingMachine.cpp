// BookVendingMachine.cpp : main project file.

#include "MainForm.h"
#include "AdminForm.h"

using namespace BookVendingMachine;

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
  if ((args->Length == 1) && (args[0] == "-admin"))
  {
	  Application::Run(gcnew AdminForm());
  }
  else
  {
	  Application::Run(gcnew MainForm());
  }
	return 0;
}
