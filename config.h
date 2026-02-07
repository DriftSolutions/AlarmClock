// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ALARMCLOCK_CONFIG_H__
#define __ALARMCLOCK_CONFIG_H__

class AlarmConfig {
private:
	DSL_Mutex * hMutex = NULL;
	bool mutex_is_ours = false;

	map<string, string> mapArgs;
	map<string, vector<string> > mapMultiArgs;

	void InterpretNegativeSetting(string name, map<string, string>& mapSettingsRet);
public:
	AlarmConfig(DSL_Mutex * mutex = NULL) {
		if (mutex != NULL) {
			hMutex = mutex;
		} else {
			hMutex = new DSL_Mutex();
			mutex_is_ours = true;
		}
	}

	~AlarmConfig() {
		if (mutex_is_ours) {
			delete hMutex;
		}
	}
	void ParseParameters(int argc, const char* const argv[]);
	void ReadConfigFile();
	/* This lets you read a config file outside of the global config system if wanted */
	void ReadConfigFile(const string& fn, map<string, string>& mapSettingsRet, map<string, vector<string> >& mapMultiSettingsRet);

	size_t CountMultiArgs(const string& strArg);
	vector<string> GetMultiArgs(const string& strArg);

	/**
	* Return string argument or default value
	*
	* @param strArg Argument to get (e.g. "-foo")
	* @param default (e.g. "1")
	* @return command-line argument or default value
	*/
	string GetArg(const string& strArg, const string& strDefault = "");

	/**
	* Return integer argument or default value
	*
	* @param strArg Argument to get (e.g. "-foo")
	* @param default (e.g. 1)
	* @return command-line argument (0 if invalid number) or default value
	*/
	int64_t GetIntArg(const string& strArg, int64_t nDefault = 0);

	/**
	* Return boolean argument or default value
	*
	* @param strArg Argument to get (e.g. "-foo")
	* @param default (true or false)
	* @return command-line argument or default value
	*/
	bool GetBoolArg(const string& strArg, bool fDefault = false);

	/**
	* Set an argument if it doesn't already have a value
	*
	* @param strArg Argument to set (e.g. "-foo")
	* @param strValue Value (e.g. "1")
	* @return true if argument gets set, false if it already had a value
	*/
	bool SetArg(const string& strArg, const string& strValue);

	/**
	* Set an argument if it doesn't already have a value
	*
	* @param strArg Argument to set (e.g. "-foo")
	* @param iValue Value (e.g. 1)
	* @return true if argument gets set, false if it already had a value
	*/
	bool SetIntArg(const string& strArg, int64_t iValue);

	/**
	* Set a boolean argument if it doesn't already have a value
	*
	* @param strArg Argument to set (e.g. "-foo")
	* @param fValue Value (e.g. false)
	* @return true if argument gets set, false if it already had a value
	*/
	bool SetBoolArg(const string& strArg, bool fValue);

	string GetDefaultDataDir();
	string GetDataDir();
	string GetDataDirFile(const string& fn);
	string GetConfigFile();

	uint16 GetNetworkPort();
};

#endif // __LIBSHIP_CONFIG_H__
