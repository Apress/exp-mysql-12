#pragma once
#pragma unmanaged
#include <stdio.h>

class DBEngine
{
private:
  bool mysqlError;
public:
  DBEngine(void);
  const char *GetError();
  bool Error();
  void Initialize();
  void Shutdown();
  char *GetSetting(char *Field);
  char *GetBookFieldStr(int Slot, char *Field);
  char *GetBookFieldText(int Slot, char *Field);
  int GetBookFieldInt(int Slot, char *Field);
  int GetQty(int Slot);
  void VendBook(char *ISBN);
  void StartQuery(char *QueryStatement);
  void RunQuery(char *QueryStatement);
  int GetNext();
  char *GetField(int fldNum);
  ~DBEngine(void);
};
#pragma managed
