// IniFile.cpp:  Implementation of the CIniFile class.
// Written by:   Adam Clauss
// Email: cabadam@houston.rr.com
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//////////////////////////////////////////////////////////////////////
#include "inifile.h"

#include <stdarg.h>
#include <stdlib.h>

namespace msec {

    CIniFile::CIniFile(string const iniPath)
    {
        Path( iniPath);
        m_bCaseInsensitive = true;
    }
    
    bool CIniFile::ReadFile()
    {
        // Normally you would use ifstream, but the SGI CC compiler has
        // a few bugs with ifstream. So ... fstream used.
        fstream f;
        string   line;
        string   keyname, valuename, value;
        string::size_type pLeft, pRight;
        
        f.open( m_strPath.c_str(), ios::in);
        if ( f.fail())
            return false;
        
        while( getline( f, line)) {

            //add begin: 当读取为空时，跳过。
            if (0 == line.length())
                continue;
            //add end: 当读取为空时，跳过。

            // To be compatible with Win32, check for existence of '\r'.
            // Win32 files have the '\r' and Unix files don't at the end of a line.
            // Note that the '\r' will be written to INI files from
            // Unix so that the created INI file can be read under Win32
            // without change.
            if ( line[line.length() - 1] == '\r')
                line = line.substr( 0, line.length() - 1);
            
            if ( line.length()) {
                // Check that the user hasn't openned a binary file by checking the first
                // character of each line!
                if ( !isprint( line[0])) {
                    printf( "Failing on char %d\n", line[0]);
                    f.close();
                    return false;
                }
                if (( pLeft = line.find_first_of(";#[=")) != string::npos) {
                    switch ( line[pLeft]) {
                    case '[':
                        if ((pRight = line.find_last_of("]")) != string::npos &&
                            pRight > pLeft) {
                            keyname = line.substr( pLeft + 1, pRight - pLeft - 1);
                            AddKeyName( keyname);
                        }
                        break;
                        
                    case '=':
                        valuename = line.substr( 0, pLeft);
                        value = line.substr( pLeft + 1);
                        SetValue( keyname, valuename, value);
                        break;
                        
                    case ';':
                    case '#':
                        if ( !names.size())
                            HeaderComment( line.substr( pLeft + 1));
                        else
                            KeyComment( keyname, line.substr( pLeft + 1));
                        break;
                    }
                }
            }
        }
        
        f.close();
        if ( names.size())
            return true;
        return false;
    }
    
    //bOpenMode true-写 false-追加
    bool CIniFile::WriteFile(bool bOpenMode)
    {
        uint32_t commentID, keyID, valueID;
        // Normally you would use ofstream, but the SGI CC compiler has
        // a few bugs with ofstream. So ... fstream used.
        fstream f;
        
        if (bOpenMode == true)
        {
            f.open( m_strPath.c_str(), ios::out);
        }else
        {
            f.open( m_strPath.c_str(), ios::out | ios::app);
        }
        if ( f.fail())
            return false;
        
        // Write header comments.
        for (commentID = 0; commentID < comments.size(); ++commentID)
            f << ';' << comments[commentID] << endl;
        if ( comments.size())
            f << endl;
        
        // Write m_vecKeys and values.
        for ( keyID = 0; keyID < m_vecKeys.size(); ++keyID) {
            f << '[' << names[keyID] << ']' << endl;
            // Comments.
            for ( commentID = 0; commentID < m_vecKeys[keyID].comments.size(); ++commentID)
                f << ';' << m_vecKeys[keyID].comments[commentID] << endl;
            // Values.
            for ( valueID = 0; valueID < m_vecKeys[keyID].names.size(); ++valueID)
                f << m_vecKeys[keyID].names[valueID] << '=' << m_vecKeys[keyID].values[valueID] << endl;
            f << endl;
        }
        f.close();
        
        return true;
    }
    
    int CIniFile::FindKey( string const keyname) const
    {
        for ( uint32_t keyID = 0; keyID < names.size(); ++keyID)
        {
            if ( CheckCase( names[keyID]) == CheckCase( keyname))
                return int(keyID);
        }
        return noID;
    }
    
    int CIniFile::FindValue( const uint32_t keyID, string const valuename) const
    {
        if ( !m_vecKeys.size() || keyID >= m_vecKeys.size())
            return noID;
        
        for ( uint32_t valueID = 0; valueID < m_vecKeys[keyID].names.size(); ++valueID)
            if ( CheckCase( m_vecKeys[keyID].names[valueID]) == CheckCase( valuename))
                return int(valueID);
            return noID;
    }
    
    uint32_t CIniFile::AddKeyName( string const keyname)
    {
        names.resize( names.size() + 1, keyname);
        m_vecKeys.resize( m_vecKeys.size() + 1);
        return names.size() - 1;
    }
    
    string CIniFile::KeyName( const uint32_t keyID) const
    {
        if ( keyID < names.size())
            return names[keyID];
        else
            return "";
    }
    
    uint32_t CIniFile::NumValues( const uint32_t keyID)
    {
        if ( keyID < m_vecKeys.size())
            return m_vecKeys[keyID].names.size();
        return 0;
    }
    
    uint32_t CIniFile::NumValues( string const keyname)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return 0;
        return m_vecKeys[keyID].names.size();
    }
    
    string CIniFile::ValueName( const uint32_t keyID, const uint32_t valueID) const
    {
        if ( keyID < m_vecKeys.size() && valueID < m_vecKeys[keyID].names.size())
            return m_vecKeys[keyID].names[valueID];
        return "";
    }
    
    string CIniFile::ValueName( string const keyname, const uint32_t valueID) const
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return "";
        return ValueName( keyID, valueID);
    }
    
    bool CIniFile::SetValue( const uint32_t keyID, const uint32_t valueID, string const value)
    {
        if ( keyID < m_vecKeys.size() && valueID < m_vecKeys[keyID].names.size())
            m_vecKeys[keyID].values[valueID] = value;
        
        return false;
    }
    
    bool CIniFile::SetValue( string const keyname, string const valuename, string const value, bool const create)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID) {
            if ( create)
                keyID = int( AddKeyName( keyname));
            else
                return false;
        }
        
        int valueID = FindValue( uint32_t(keyID), valuename);
        if ( valueID == noID) {
            if ( !create)
                return false;
            m_vecKeys[keyID].names.resize( m_vecKeys[keyID].names.size() + 1, valuename);
            m_vecKeys[keyID].values.resize( m_vecKeys[keyID].values.size() + 1, value);
        } else
            m_vecKeys[keyID].values[valueID] = value;
        
        return true;
    }
    
    bool CIniFile::SetValueI( string const keyname, string const valuename, int const value, bool const create)
    {
        char svalue[MAX_VALUEDATA];
        
        sprintf( svalue, "%d", value);
        return SetValue( keyname, valuename, svalue);
    }
    
    bool CIniFile::SetValueF( string const keyname, string const valuename, const float value, bool const create)
    {
        char svalue[MAX_VALUEDATA];
        
        sprintf( svalue, "%f", value);
        return SetValue( keyname, valuename, svalue);
    }
    
    bool CIniFile::SetValueV( string const keyname, string const valuename, char *format, ...)
    {
        va_list args;
        char value[MAX_VALUEDATA];
        
        va_start( args, format);
        vsprintf( value, format, args);
        va_end( args);
        return SetValue( keyname, valuename, value);
    }
    
    string CIniFile::GetValue( const uint32_t keyID, const uint32_t valueID, string const defValue) const
    {
        if ( keyID < m_vecKeys.size() && valueID < m_vecKeys[keyID].names.size())
            return m_vecKeys[keyID].values[valueID];
        return defValue;
    }
    
    string CIniFile::GetValue( string const keyname, string const valuename, string const defValue) const
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return defValue;
        
        int valueID = FindValue( uint32_t(keyID), valuename);
        if ( valueID == noID)
            return defValue;
        
        return m_vecKeys[keyID].values[valueID];
    }
    
    int CIniFile::GetValueI(string const keyname, string const valuename, int const defValue) const
    {
        char svalue[MAX_VALUEDATA];
        
        sprintf( svalue, "%d", defValue);
        return atoi( GetValue( keyname, valuename, svalue).c_str()); 
    }
    
    double CIniFile::GetValueF(string const keyname, string const valuename, const float defValue) const
    {
        char svalue[MAX_VALUEDATA];
        
        sprintf( svalue, "%f", defValue);
        return atof( GetValue( keyname, valuename, svalue).c_str()); 
    }
    
    // 16 variables may be a bit of over kill, but hey, it's only code.
    uint32_t CIniFile::GetValueV( string const keyname, string const valuename, char *format,
        void *v1, void *v2, void *v3, void *v4,
        void *v5, void *v6, void *v7, void *v8,
        void *v9, void *v10, void *v11, void *v12,
        void *v13, void *v14, void *v15, void *v16)
    {
        string   value;
        // va_list  args;
        uint32_t nVals;
        
        
        value = GetValue( keyname, valuename);
        if ( !value.length())
            return false;
        // Why is there not vsscanf() function. Linux man pages say that there is
        // but no compiler I've seen has it defined. Bummer!
        //
        // va_start( args, format);
        // nVals = vsscanf( value.c_str(), format, args);
        // va_end( args);
        
        nVals = sscanf( value.c_str(), format,
            v1, v2, v3, v4, v5, v6, v7, v8,
            v9, v10, v11, v12, v13, v14, v15, v16);
        
        return nVals;
    }
    
    bool CIniFile::DeleteValue( string const keyname, string const valuename)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return false;
        
        int valueID = FindValue( uint32_t(keyID), valuename);
        if ( valueID == noID)
            return false;
        
        // This looks strange, but is neccessary.
        vector<string>::iterator npos = m_vecKeys[keyID].names.begin() + valueID;
        vector<string>::iterator vpos = m_vecKeys[keyID].values.begin() + valueID;
        m_vecKeys[keyID].names.erase( npos, npos + 1);
        m_vecKeys[keyID].values.erase( vpos, vpos + 1);
        
        return true;
    }
    
    bool CIniFile::DeleteKey( string const keyname)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return false;
        
        // Now hopefully this destroys the vector lists within m_vecKeys.
        // Looking at <vector> source, this should be the case using the destructor.
        // If not, I may have to do it explicitly. Memory leak check should tell.
        // memleak_test.cpp shows that the following not required.
        //m_vecKeys[keyID].names.clear();
        //m_vecKeys[keyID].values.clear();
        
        vector<string>::iterator npos = names.begin() + keyID;
        vector<key>::iterator    kpos = m_vecKeys.begin() + keyID;
        names.erase( npos, npos + 1);
        m_vecKeys.erase( kpos, kpos + 1);
        
        return true;
    }
    
    void CIniFile::Erase()
    {
        // This loop not needed. The vector<> destructor seems to do
        // all the work itself. memleak_test.cpp shows this.
        //for ( uint32_t i = 0; i < m_vecKeys.size(); ++i) {
        //  m_vecKeys[i].names.clear();
        //  m_vecKeys[i].values.clear();
        //}
        names.clear();
        m_vecKeys.clear();
        comments.clear();
    }
    
    void CIniFile::HeaderComment( string const comment)
    {
        comments.resize( comments.size() + 1, comment);
    }
    
    string CIniFile::HeaderComment( const uint32_t commentID) const
    {
        if ( commentID < comments.size())
            return comments[commentID];
        return "";
    }
    
    bool CIniFile::DeleteHeaderComment( uint32_t commentID)
    {
        if ( commentID < comments.size()) {
            vector<string>::iterator cpos = comments.begin() + commentID;
            comments.erase( cpos, cpos + 1);
            return true;
        }
        return false;
    }
    
    uint32_t CIniFile::NumKeyComments( const uint32_t keyID) const
    {
        if ( keyID < m_vecKeys.size())
            return m_vecKeys[keyID].comments.size();
        return 0;
    }
    
    uint32_t CIniFile::NumKeyComments( string const keyname) const
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return 0;
        return m_vecKeys[keyID].comments.size();
    }
    
    bool CIniFile::KeyComment( const uint32_t keyID, string const comment)
    {
        if ( keyID < m_vecKeys.size()) {
            m_vecKeys[keyID].comments.resize( m_vecKeys[keyID].comments.size() + 1, comment);
            return true;
        }
        return false;
    }
    
    bool CIniFile::KeyComment( string const keyname, string const comment)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return false;
        return KeyComment( uint32_t(keyID), comment);
    }
    
    string CIniFile::KeyComment( const uint32_t keyID, const uint32_t commentID) const
    {
        if ( keyID < m_vecKeys.size() && commentID < m_vecKeys[keyID].comments.size())
            return m_vecKeys[keyID].comments[commentID];
        return "";
    }
    
    string CIniFile::KeyComment( string const keyname, const uint32_t commentID) const
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return "";
        return KeyComment( uint32_t(keyID), commentID);
    }
    
    bool CIniFile::DeleteKeyComment( const uint32_t keyID, const uint32_t commentID)
    {
        if ( keyID < m_vecKeys.size() && commentID < m_vecKeys[keyID].comments.size()) {
            vector<string>::iterator cpos = m_vecKeys[keyID].comments.begin() + commentID;
            m_vecKeys[keyID].comments.erase( cpos, cpos + 1);
            return true;
        }
        return false;
    }
    
    bool CIniFile::DeleteKeyComment( string const keyname, const uint32_t commentID)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return false;
        return DeleteKeyComment( uint32_t(keyID), commentID);
    }
    
    bool CIniFile::DeleteKeyComments( const uint32_t keyID)
    {
        if ( keyID < m_vecKeys.size()) {
            m_vecKeys[keyID].comments.clear();
            return true;
        }
        return false;
    }
    
    bool CIniFile::DeleteKeyComments( string const keyname)
    {
        int keyID = FindKey( keyname);
        if ( keyID == noID)
            return false;
        return DeleteKeyComments( uint32_t(keyID));
    }
    
    string CIniFile::CheckCase( string s) const
    {
        if ( m_bCaseInsensitive)
            for ( string::size_type i = 0; i < s.length(); ++i)
                s[i] = tolower(s[i]);
            return s;
    }


}

