// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "alarmclock.h"

void AlarmConfig::InterpretNegativeSetting(string name, map<string, string>& mapSettingsRet) {
	// interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
	if (name.find("-no") == 0) {
		string positive("-");
		positive.append(name.begin() + 3, name.end());
		if (mapSettingsRet.count(positive) == 0) {
			bool value = !GetBoolArg(name);
			mapSettingsRet[positive] = (value ? "1" : "0");
		}
	}
}

void AlarmConfig::ParseParameters(int argc, const char* const argv[]) {
	AutoMutex(*hMutex);
	mapArgs.clear();
	mapMultiArgs.clear();
	for (int i = 1; i < argc; i++) {
		string str(argv[i]);
		string strValue;
		size_t is_index = str.find('=');
		if (is_index != string::npos) {
			strValue = str.substr(is_index + 1);
			str = str.substr(0, is_index);
		}
#ifdef WIN32
		if (str[0] == '/')
			str = "-" + str.substr(1);
#endif
		if (str[0] != '-')
			break;

		mapArgs[str] = strValue;
		mapMultiArgs[str].push_back(strValue);
	}

	// New 0.6 features:
	for (auto entry = mapArgs.begin(); entry != mapArgs.end(); entry++) {
		string name = entry->first;

		//  interpret --foo as -foo (as long as both are not set)
		if (name.find("--") == 0) {
			string singleDash(name.begin() + 1, name.end());
			if (mapArgs.count(singleDash) == 0)
				mapArgs[singleDash] = entry->second;
			name = singleDash;
		}

		// interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
		InterpretNegativeSetting(name, mapArgs);
	}
}

size_t AlarmConfig::CountMultiArgs(const string& strArg) {
	AutoMutex(*hMutex);
	return mapMultiArgs.count(strArg);
}

vector<string> AlarmConfig::GetMultiArgs(const string& strArg) {
	AutoMutex(*hMutex);
	return mapMultiArgs[strArg];
}

string AlarmConfig::GetArg(const string& strArg, const string& strDefault) {
	AutoMutex(*hMutex);
	if (mapArgs.count(strArg))
		return mapArgs[strArg];
	return strDefault;
}

int64_t AlarmConfig::GetIntArg(const string& strArg, int64_t nDefault) {
	AutoMutex(*hMutex);
	if (mapArgs.count(strArg))
		return atoi64(mapArgs[strArg].c_str());
	return nDefault;
}

bool AlarmConfig::GetBoolArg(const string& strArg, bool fDefault) {
	AutoMutex(*hMutex);
#ifdef _DEBUG
	/*
	if (strncmp(strArg.c_str(), "-debug", 6) == 0) {
		return true;
	}
	*/
#endif
	if (mapArgs.count(strArg)) {
		if (mapArgs[strArg].empty())
			return true;
		return (atoi(mapArgs[strArg].c_str()) != 0);
	}
	return fDefault;
}

bool AlarmConfig::SetArg(const string& strArg, const string& strValue) {
	AutoMutex(*hMutex);
	mapArgs[strArg] = strValue;
	return true;
}

bool AlarmConfig::SetIntArg(const string& strArg, int64_t iValue) {
	AutoMutex(*hMutex);

	char strValue[64] = { 0 };
	snprintf(strValue, sizeof(strValue), I64FMT, iValue);
	mapArgs[strArg] = strValue;
	return true;
}

bool AlarmConfig::SetBoolArg(const string& strArg, bool fValue) {
	AutoMutex(*hMutex);
	if (fValue)
		return SetArg(strArg, string("1"));
	else
		return SetArg(strArg, string("0"));
}

void AlarmConfig::ReadConfigFile(const string& fn, map<string, string>& mapSettingsRet, map<string, vector<string> >& mapMultiSettingsRet) {
	//	AutoMutex(*hMutex);

	FILE * fp = fopen(fn.c_str(), "rb");
	if (fp == NULL) {
		return;
	}

	char buf[1024];

	while (!feof(fp) && fgets(buf, sizeof(buf), fp)) {
		strtrim(buf);
		if (buf[0] == 0 || buf[0] == '#' || buf[0] == '/') {
			continue;
		}
		char * val = strchr(buf, '=');
		if (val == NULL) {
			continue;
		}
		*val = 0;
		val++;

		// Don't overwrite existing settings so command line settings override bitcoin.conf
		string strKey = string("-") + buf;
		if (mapSettingsRet.count(strKey) == 0) {
			mapSettingsRet[strKey] = val;
			// interpret nofoo=1 as foo=0 (and nofoo=0 as foo=1) as long as foo not set)
			InterpretNegativeSetting(strKey, mapSettingsRet);
		}
		mapMultiSettingsRet[strKey].push_back(val);
	}

	fclose(fp);
}

string AlarmConfig::GetConfigFile() {
	static string fn;
	if (fn.empty()) {
		string conf = GetArg("-conf");
		if (conf.empty()) {
			conf = "alarmclock.conf";
		}
		if (strstr(conf.c_str(), "/") == NULL && strstr(conf.c_str(), "\\") == NULL) {
			fn = GetDataDirFile(conf);
		} else {
			fn = conf;
		}
	}
	return fn;
}

void AlarmConfig::ReadConfigFile() {
	AutoMutex(*hMutex);
	string conf = GetConfigFile();
	if (access(conf.c_str(), 0) == 0) {
		ReadConfigFile(conf, mapArgs, mapMultiArgs);
	} else {
		printf("Warning: No '%s' config file found!\n", conf.c_str());
	}
}

string AlarmConfig::GetDefaultDataDir() {
	return "./data/";
}

string AlarmConfig::GetDataDir() {
	AutoMutex(*hMutex);
	static string dir;
	if (dir.length() == 0) {
		dir = GetArg("-datadir", GetDefaultDataDir());
		if (dir.length() && dir.substr(dir.length() - 1, 1) != PATH_SEPS && dir.substr(dir.length() - 1, 1) != "/") {
			dir += PATH_SEPS;
		}
		if (access(dir.c_str(), 0) != 0) {
			dsl_mkdir(dir.c_str(), 0700);
		}
	}
	return dir;
}

string AlarmConfig::GetDataDirFile(const string& fn) {
	string ret = GetDataDir();
	ret += fn;
	return ret;
}

/*
uint16 AlarmConfig::GetNetworkPort() {
	int64_t port = GetIntArg("-port", SHIP_BUS_DEFAULT_PORT);
	if (port > 0 && port < 65536) {
		return (uint16)port;
	}
	return SHIP_BUS_DEFAULT_PORT;
}
*/
