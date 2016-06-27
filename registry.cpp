#include <windows.h>

extern "C"{

void CreateSetKeyExtension(void);
void CreateSetKeyDescription(void);
void CreateSetKeyCommandLine(const char*  exeline);

};
 
void CreateSetKeyExtension(void)
{
 
 unsigned long IsNew = 0;
 HKEY hregkey;

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".dem", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, "Quake");
  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
 
  RegCloseKey(hregkey);
 }

}

void CreateSetKeyDescription(void)
{
 
 unsigned long IsNew = 0;
 HKEY hregkey;

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, "Quake", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, "Quake Demo");
  //RegSetValueEx(hregkey, "String", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
 
  RegCloseKey(hregkey);
 }

}

void CreateSetKeyCommandLine(const char* exeline)
{
	// Must send something like c:\quake\quake.exe %1
 unsigned long IsNew = 0;
 HKEY hregkey;

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, "Quake\\shell\\open\\command", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, exeline);

  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  RegCloseKey(hregkey);
 }

}

