// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Drift Solutions

#include "alarmclock.h"
#include "json.hpp"

using json = nlohmann::json;

size_t jsonrpc_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	DSL_BUFFER* buf = (DSL_BUFFER*)userdata;
	buffer_append(buf, ptr, nmemb);
	return nmemb;
}

string jsonAsString(const json& x, const string& def) {
	if (x.is_string()) {
		return x.get<std::string>();
	} else if (x.is_boolean()) {
		return x.get<bool>() ? "1" : "0";
	} else if (x.is_number_integer()) {
		char buf[128];
		snprintf(buf, sizeof(buf), "%llu", x.get<int64_t>());
		return buf;
	} else if (x.is_number_float()) {
		char buf[128];
		snprintf(buf, sizeof(buf), "%.8f", x.get<double>());
		return buf;
	}
	return def;
}

class Home_Assistant_Entity {
public:
	string state;
	map<string, string> attr;

	string getAttribute(const string& name, const string& def = "") {
		auto x = attr.find(name);
		if (x != attr.end()) {
			return x->second;
		}
		return def;
	}
};

class Home_Assistant_Info {
public:
	bool running = false;
	map<string, Home_Assistant_Entity> entities;
};

class Home_Assistant_Client {
private:
	string base_url, token;
	CURL* cHandle = NULL;
	CURL* cHandlePost = NULL;
	DSL_BUFFER jsonrpc_write_buffer;
	string _error;

public:
	void setError(const string& msg) {
		_error = msg;
		printf("%s\n", msg.c_str());
	}
	const string& error = _error;

	Home_Assistant_Client(const string& pbase_url, const string& ptoken) {
		base_url = pbase_url;
		token = ptoken;
		memset(&jsonrpc_write_buffer, 0, sizeof(jsonrpc_write_buffer));
		buffer_init(&jsonrpc_write_buffer);
	}

	~Home_Assistant_Client() {
		if (cHandle != NULL) {
			curl_easy_cleanup(cHandle);
			cHandle = NULL;
		}
		if (cHandlePost != NULL) {
			curl_easy_cleanup(cHandlePost);
			cHandlePost = NULL;
		}
		buffer_free(&jsonrpc_write_buffer);
	}

	bool perform_get(const string& path, json& out) {
		out.clear();

		if (cHandle == NULL) {
			cHandle = curl_easy_init();
			if (cHandle == NULL) {
				return false;
			}

			stringstream url;
			url << base_url << "api/states";
			curl_easy_setopt(cHandle, CURLOPT_URL, url.str().c_str());

			curl_easy_setopt(cHandle, CURLOPT_NOPROGRESS, true);
			curl_easy_setopt(cHandle, CURLOPT_NOSIGNAL, true);
			curl_easy_setopt(cHandle, CURLOPT_MAXREDIRS, 2);
			curl_easy_setopt(cHandle, CURLOPT_TIMEOUT, 60);
			curl_easy_setopt(cHandle, CURLOPT_CONNECTTIMEOUT, 20);
			curl_easy_setopt(cHandle, CURLOPT_FAILONERROR, false);
			curl_easy_setopt(cHandle, CURLOPT_WRITEFUNCTION, jsonrpc_write_callback);
			curl_easy_setopt(cHandle, CURLOPT_WRITEDATA, &jsonrpc_write_buffer);
#ifdef DEBUG
			curl_easy_setopt(cHandle, CURLOPT_SSL_VERIFYPEER, false);
#endif
			curl_easy_setopt(cHandle, CURLOPT_USERAGENT, "drift_alarm_clock");
			auto headers = curl_slist_append(NULL, "Accept: application/json");
			headers = curl_slist_append(NULL, mprintf("Authorization: Bearer %s", token.c_str()).c_str());
			curl_easy_setopt(cHandle, CURLOPT_HTTPHEADER, headers);
		}

		stringstream url;
		url << base_url << path;
		curl_easy_setopt(cHandle, CURLOPT_URL, url.str().c_str());

		buffer_clear(&jsonrpc_write_buffer);

		bool bret = false;
		auto ret = curl_easy_perform(cHandle);
		if (ret == CURLE_OK && jsonrpc_write_buffer.len > 0) {
			long code = 0;
			curl_easy_getinfo(cHandle, CURLINFO_RESPONSE_CODE, &code);

			buffer_append_int<uint8_t>(&jsonrpc_write_buffer, 0);

			if (code >= 200 && code < 300) {
				try {
					out = json::parse(jsonrpc_write_buffer.data);
					if (out.is_array() || out.is_object()) {
						bret = true;
					} else {
						out.clear();
					}
				}
				catch (...) {
					out.clear();
				}
			} else {
				setError(mprintf("HTTP error (%d): %s", code, curl_easy_strerror(ret)));
				printf("Reply was: %s\n", jsonrpc_write_buffer.data);
			}
		} else {
			setError(mprintf("Error connecting: %s", curl_easy_strerror(ret)));
		}

		buffer_clear(&jsonrpc_write_buffer);
		return bret;
	}

	bool perform_post(const string& path, const json& payload, json& out) {
		out.clear();

		if (cHandlePost == NULL) {
			cHandlePost = curl_easy_init();
			if (cHandlePost == NULL) {
				return false;
			}

			stringstream url;
			url << base_url << "api/states";
			curl_easy_setopt(cHandlePost, CURLOPT_URL, url.str().c_str());

			curl_easy_setopt(cHandlePost, CURLOPT_NOPROGRESS, true);
			curl_easy_setopt(cHandlePost, CURLOPT_NOSIGNAL, true);
			curl_easy_setopt(cHandlePost, CURLOPT_MAXREDIRS, 2);
			curl_easy_setopt(cHandlePost, CURLOPT_TIMEOUT, 60);
			curl_easy_setopt(cHandlePost, CURLOPT_CONNECTTIMEOUT, 20);
			curl_easy_setopt(cHandlePost, CURLOPT_FAILONERROR, false);
			curl_easy_setopt(cHandlePost, CURLOPT_HTTPPOST, true);
			curl_easy_setopt(cHandlePost, CURLOPT_WRITEFUNCTION, jsonrpc_write_callback);
			curl_easy_setopt(cHandlePost, CURLOPT_WRITEDATA, &jsonrpc_write_buffer);
#ifdef DEBUG
			curl_easy_setopt(cHandlePost, CURLOPT_SSL_VERIFYPEER, false);
#endif
			curl_easy_setopt(cHandlePost, CURLOPT_USERAGENT, "drift_alarm_clock");
			auto headers = curl_slist_append(NULL, "Content-Type: application/json");
			headers = curl_slist_append(NULL, "Accept: application/json");
			headers = curl_slist_append(NULL, mprintf("Authorization: Bearer %s", token.c_str()).c_str());
			curl_easy_setopt(cHandlePost, CURLOPT_HTTPHEADER, headers);
		}

		stringstream url;
		url << base_url << path;
		curl_easy_setopt(cHandlePost, CURLOPT_URL, url.str().c_str());

		string json = payload.dump();
		curl_easy_setopt(cHandlePost, CURLOPT_POSTFIELDSIZE, json.length());
		curl_easy_setopt(cHandlePost, CURLOPT_COPYPOSTFIELDS, json.c_str());

		buffer_clear(&jsonrpc_write_buffer);

		bool bret = false;
		auto ret = curl_easy_perform(cHandlePost);
		if (ret == CURLE_OK && jsonrpc_write_buffer.len > 0) {
			long code = 0;
			curl_easy_getinfo(cHandlePost, CURLINFO_RESPONSE_CODE, &code);

			buffer_append_int<uint8_t>(&jsonrpc_write_buffer, 0);

			if (code >= 200 && code < 300) {
				try {
					out = json::parse(jsonrpc_write_buffer.data);
					if (out.is_array() || out.is_object()) {
						bret = true;
					} else {
						out.clear();
					}
				}
				catch (...) {
					out.clear();
				}
			} else {
				setError(mprintf("HTTP error (%d): %s", code, curl_easy_strerror(ret)));
				printf("Reply was: %s\n", jsonrpc_write_buffer.data);
			}
		} else {
			setError(mprintf("Error connecting: %s", curl_easy_strerror(ret)));
		}

		buffer_clear(&jsonrpc_write_buffer);
		return bret;
	}

	bool GetEntities(Home_Assistant_Info& info) {
		json reply;
		if (!perform_get("api/states", reply)) {
			return false;
		}

		for (const auto& e : reply) {
			string name = e.contains("entity_id") ? e["entity_id"].get<string>() : "";
			string state = e.contains("state") ? e["state"].get<string>() : "";
			if (!name.empty() && !state.empty()) {
				Home_Assistant_Entity entity;
				entity.state = state;
				if (e.contains("attributes") && e["attributes"].is_object()) {
					for (auto& a : e["attributes"].items()) {
						entity.attr[a.key()] = jsonAsString(a.value(), "");
					}
				}
				info.entities[name] = entity;
			}
		}

		info.running = true;
		return true;
	}

	bool UpdateStateJSON(const string& entity_id, const json& payload) {
		json reply;
		if (!perform_post("api/states/" + entity_id, payload, reply)) {
			return false;
		}

		return true;
	}

	bool UpdateStateStr(const string& entity_id, const string& state) {
		json payload;
		payload["state"] = state;
		return UpdateStateJSON(entity_id, payload);
	}

	bool UpdateStateBool(const string& entity_id, bool state) {
		json payload;
		payload["state"] = state;
		return UpdateStateJSON(entity_id, payload);
	}
};

DSL_Mutex infoMutex;
Home_Assistant_Info info;
bool ha_had_update = false;

DSL_DEFINE_THREAD(HomeAssistantUpdateInfoThread) {
	DSL_THREAD_START
	Home_Assistant_Client* cli = new Home_Assistant_Client(config.home_assistant.url, config.home_assistant.token);
	
	bool send_updates = config.home_assistant.needToSendUpdates();
	bool last_sent_alarming = false;
	bool last_sent_alarm_enabled = false;
	time_t last_sent_next_alarm = 0;
	int64 last_sent = 0;

	int64 last_update = 0;
	while (!config.shutdown_now) {
		if (send_updates) {
			bool do_scheduled_send = (time(NULL) - last_sent >= 600);
			bool had_error = false;
			bool did_send = false;

			if (!config.home_assistant.alarm_entity.empty() && (config.alarming != last_sent_alarming || do_scheduled_send)) {
				bool tmp = config.alarming;
				if (cli->UpdateStateStr(config.home_assistant.alarm_entity, tmp ? "on" : "off")) {
					printf("Sent Home Assistant new alarm status: %s\n", tmp ? "on" : "off");
					last_sent_alarming = tmp;
					did_send = true;
				} else {
					printf("Error sending Home Assistant new alarm status: %s -> %s\n", tmp ? "on" : "off", cli->error.c_str());
					statusbar_set(SB_HOME_ASSISTANT_SEND, mprintf("HA Error: %s", cli->error.c_str()));
					had_error = true;
				}
			}
			if (!config.home_assistant.alarm_enabled_entity.empty() && (config.options.enable_alarm != last_sent_alarm_enabled || do_scheduled_send)) {
				bool tmp = config.options.enable_alarm;
				if (cli->UpdateStateStr(config.home_assistant.alarm_enabled_entity, tmp ? "on" : "off")) {
					printf("Sent Home Assistant new alarm enabled status: %s\n", tmp ? "on" : "off");
					last_sent_alarm_enabled = tmp;
					did_send = true;
				} else {
					printf("Error sending Home Assistant new alarm enabled status: %s -> %s\n", tmp ? "on" : "off", cli->error.c_str());
					statusbar_set(SB_HOME_ASSISTANT_SEND, mprintf("HA Error: %s", cli->error.c_str()));
					had_error = true;
				}
			}

			if (!config.home_assistant.next_alarm_entity.empty()) {
				time_t next_alarm = config.options.GetNextAlarmTime();
				if (next_alarm != last_sent_next_alarm || do_scheduled_send) {
					char buf[128];
					struct tm tm;
					localtime_r(&next_alarm, &tm);
					strftime(buf, sizeof(buf), "%FT%T", &tm);
					if (cli->UpdateStateStr(config.home_assistant.next_alarm_entity, buf)) {
						printf("Sent Home Assistant new next alarm time: %s\n", buf);
						last_sent_next_alarm = next_alarm;
						did_send = true;
					} else {
						printf("Error sending Home Assistant new next alarm time: %s -> %s\n", buf, cli->error.c_str());
						statusbar_set(SB_HOME_ASSISTANT_SEND, mprintf("HA Error: %s", cli->error.c_str()));
						had_error = true;
					}
				}
			}

			if (did_send && !had_error) {
				statusbar_set(SB_HOME_ASSISTANT_SEND, mprintf("HA: Sent %s", ts_to_str(time(NULL)).c_str()));
				last_sent = time(NULL);
			}
		}

		if (time(NULL) - last_update >= 30) {
			Home_Assistant_Info info;
			if (cli->GetEntities(info) && info.running) {
				statusbar_set(SB_HOME_ASSISTANT_RECV, mprintf("HA: Updated %s", ts_to_str(time(NULL)).c_str()));
				AutoMutex(infoMutex);
				::info = std::move(info);
				ha_had_update = true;
			} else {
				statusbar_set(SB_HOME_ASSISTANT_RECV, mprintf("HA Error: %s", cli->error.c_str()));
			}
			last_update = time(NULL);
		}

		safe_sleep(1);
	}

	delete cli;
	DSL_THREAD_END
}

void start_home_assistant() {
	if (config.home_assistant.isValid()) {
		DSL_StartThread(HomeAssistantUpdateInfoThread, NULL, "Home Assistant Info Updater");
	}
}

string ucwords(const string& str) {
	string ret;
	StrTokenizer st((char *)str.c_str(), ' ');
	for (size_t i = 1; i <= st.NumTok(); i++) {
		char* tmp = st.GetSingleTok(i);
		tmp[0] = toupper(tmp[0]);
		if (!ret.empty()) {
			ret += " ";
		}
		ret += tmp;
		dsl_free(tmp);
	}
	return ret;
}

void update_home_assistant() {
	static string weather_entity;
	if (weather_entity.empty()) {
		weather_entity = cfg->GetArg("-home_assistant_weather", "weather.openweathermap");
	}
	AutoMutex(infoMutex);
	if (info.running && ha_had_update) {
		config.home_assistant.info.last_update = time(NULL);

		auto x = info.entities.find("sensor.openweathermap_weather");
		if (x != info.entities.end()) {
			config.home_assistant.info.weather = ucwords(x->second.state);
		}

		x = info.entities.find(weather_entity);
		if (x != info.entities.end()) {
			if (config.home_assistant.info.weather.empty()) {
				config.home_assistant.info.weather = ucwords(x->second.state);
			}
			config.home_assistant.info.temp = atof(x->second.getAttribute("temperature").c_str());
			config.home_assistant.info.temp_unit = x->second.getAttribute("temperature_unit");
		}

		if (config.use_sun) {
			x = info.entities.find("sensor.custom_day_dark_sleep_w_o_weather");
			if (x != info.entities.end()) {
				config.home_assistant.info.is_dark = (stricmp(x->second.state.c_str(), "sleep") == 0);
				config.SetIsDark(config.home_assistant.info.is_dark);
			} else {
				x = info.entities.find("sun.sun");
				if (x != info.entities.end()) {
					double elevation = atof(x->second.getAttribute("elevation").c_str());
					config.SetIsDark(x->second.state == "below_horizon" || elevation < 0);
				}
			}
		}

		ha_had_update = false;
	}
}
