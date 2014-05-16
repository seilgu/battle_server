#include "protocol.h"

namespace hwo_protocol
{
	jsoncons::json make_request(const std::string& msg_type, const jsoncons::json& data)
	{
		jsoncons::json r;
		r["msgType"] = msg_type;
		r["data"] = data;
		return r;
	}

	jsoncons::json make_ping()
	{
		return make_request("ping", jsoncons::null_type());
	}

	jsoncons::json make_join(const std::string& name, const std::string& key)
	{
		jsoncons::json data;
		jsoncons::json botId;
		botId["name"] = name;
		botId["key"] = key;
		data["botId"] = botId;
		return make_request("joinRace", data);
	}

}  // namespace hwo_protocol
