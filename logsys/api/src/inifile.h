// IniFile.cpp:  Implementation of the CIniFile class.
// Written by:   Adam Clauss
// Email: cabadam@tamu.edu
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//////////////////////////////////////////////////////////////////////

#ifndef __CINIFILE_H__
#define __CINIFILE_H__

#define MAX_KEYNAME    128
#define MAX_VALUENAME  128
#define MAX_VALUEDATA 2048

#include <vector>
#include <string>
#include <fstream>
#include <stdint.h>
using namespace std;

namespace msec {
    
    class CIniFile
    {
    private:
        bool   m_bCaseInsensitive;
        string   m_strPath;
        struct key {
            vector<string> names;
            vector<string> values; 
            vector<string> comments;
        };
        vector<key>    m_vecKeys; 
        vector<string> names; 
        vector<string> comments;
        string CheckCase( string s) const;
        
        public:
            enum errors{ noID = -1};
            CIniFile( string const iniPath = "");
            virtual ~CIniFile()                            {}
            
            // Sets whether or not keynames and valuenames should be case sensitive.
            // The default is case insensitive.
            void CaseSensitive()                           {m_bCaseInsensitive = false;}
            void CaseInsensitive()                         {m_bCaseInsensitive = true;}
            
            // Sets path of ini file to read and write from.
            void Path(string const newPath)                {m_strPath = newPath;}
            string Path() const                            {return m_strPath;}
            void SetPath(string const newPath)             {Path( newPath);}
            
            // Reads ini file specified using path.
            // Returns true if successful, false otherwise.
            bool ReadFile();
            
            // Writes data stored in class to ini file.
            //bOpenMode true-Ð´ false-×·¼Ó
            bool WriteFile(bool bOpenMode = true); 
            
            // Deletes all stored ini data.
            void Erase();
            void Clear()                                   {Erase();}
            void Reset()                                   {Erase();}
            
            // Returns index of specified key, or noID if not found.
            int FindKey( string const keyname) const;
            
            // Returns index of specified value, in the specified key, or noID if not found.
            int FindValue( const uint32_t keyID, string const valuename) const;
            
            // Returns number of m_vecKeys currently in the ini.
            uint32_t NumKeys() const                       {return names.size();}
            uint32_t GetNumKeys() const                    {return NumKeys();}
            
            // Add a key name.
            uint32_t AddKeyName( string const keyname);
            
            // Returns key names by index.
            string KeyName( const uint32_t keyID) const;
            string GetKeyName( const uint32_t keyID) const {return KeyName(keyID);}
            
            // Returns number of values stored for specified key.
            uint32_t NumValues( const uint32_t keyID);
            uint32_t GetNumValues( const uint32_t keyID)   {return NumValues( keyID);}
            uint32_t NumValues( string const keyname);
            uint32_t GetNumValues( string const keyname)   {return NumValues( keyname);}
            
            // Returns value name by index for a given keyname or keyID.
            string ValueName( const uint32_t keyID, const uint32_t valueID) const;
            string GetValueName( const uint32_t keyID, const uint32_t valueID) const {
                return ValueName( keyID, valueID);
            }
            string ValueName( string const keyname, const uint32_t valueID) const;
            string GetValueName( string const keyname, const uint32_t valueID) const {
                return ValueName( keyname, valueID);
            }
            
            // Gets value of [keyname] valuename =.
            // Overloaded to return string, int, and double.
            // Returns defValue if key/value not found.
            string GetValue( const uint32_t keyID, const uint32_t valueID, string const defValue = "") const;
            string GetValue(string const keyname, string const valuename, string const defValue = "") const; 
            int    GetValueI(string const keyname, string const valuename, int const defValue = 0) const;
            bool   GetValueB(string const keyname, string const valuename, bool const defValue = false) const {
                return bool( GetValueI( keyname, valuename, int( defValue)));
            }
            double   GetValueF(string const keyname, string const valuename, const float defValue = 0.0) const;
            // This is a variable length formatted GetValue routine. All these voids
            // are required because there is no vsscanf() like there is a vsprintf().
            // Only a maximum of 8 variable can be read.
            uint32_t GetValueV( string const keyname, string const valuename, char *format,
                void *v1 = 0, void *v2 = 0, void *v3 = 0, void *v4 = 0,
                void *v5 = 0, void *v6 = 0, void *v7 = 0, void *v8 = 0,
                void *v9 = 0, void *v10 = 0, void *v11 = 0, void *v12 = 0,
                void *v13 = 0, void *v14 = 0, void *v15 = 0, void *v16 = 0);
            
            // Sets value of [keyname] valuename =.
            // Specify the optional paramter as false (0) if you do not want it to create
            // the key if it doesn't exist. Returns true if data entered, false otherwise.
            // Overloaded to accept string, int, and double.
            bool SetValue( const uint32_t keyID, const uint32_t valueID, string const value);
            bool SetValue( string const keyname, string const valuename, string const value, bool const create = true);
            bool SetValueI( string const keyname, string const valuename, int const value, bool const create = true);
            bool SetValueB( string const keyname, string const valuename, bool const value, bool const create = true) {
                return SetValueI( keyname, valuename, int(value), create);
            }
            bool SetValueF( string const keyname, string const valuename, const float value, bool const create = true);
            bool SetValueV( string const keyname, string const valuename, char *format, ...);
            
            // Deletes specified value.
            // Returns true if value existed and deleted, false otherwise.
            bool DeleteValue( string const keyname, string const valuename);
            
            // Deletes specified key and all values contained within.
            // Returns true if key existed and deleted, false otherwise.
            bool DeleteKey(string keyname);
            
            // Header comment functions.
            // Header comments are those comments before the first key.
            //
            // Number of header comments.
            uint32_t NumHeaderComments()                  {return comments.size();}
            // Add a header comment.
            void     HeaderComment( string const comment);
            // Return a header comment.
            string   HeaderComment( const uint32_t commentID) const;
            // Delete a header comment.
            bool     DeleteHeaderComment( uint32_t commentID);
            // Delete all header comments.
            void     DeleteHeaderComments()               {comments.clear();}
            
            // Key comment functions.
            // Key comments are those comments within a key. Any comments
            // defined within value names will be added to this list. Therefore,
            // these comments will be moved to the top of the key definition when
            // the CIniFile::WriteFile() is called.
            //
            // Number of key comments.
            uint32_t NumKeyComments( const uint32_t keyID) const;
            uint32_t NumKeyComments( string const keyname) const;
            // Add a key comment.
            bool     KeyComment( const uint32_t keyID, string const comment);
            bool     KeyComment( string const keyname, string const comment);
            // Return a key comment.
            string   KeyComment( const uint32_t keyID, const uint32_t commentID) const;
            string   KeyComment( string const keyname, const uint32_t commentID) const;
            // Delete a key comment.
            bool     DeleteKeyComment( const uint32_t keyID, const uint32_t commentID);
            bool     DeleteKeyComment( string const keyname, const uint32_t commentID);
            // Delete all comments for a key.
            bool     DeleteKeyComments( const uint32_t keyID);
            bool     DeleteKeyComments( string const keyname);
    };
}
#endif


