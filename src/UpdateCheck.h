#pragma once
#include <windows.h>
#include <string>

// Checking for a newer Bulb: asks GitHub for the newest release (betas included) and tells a window its tag.
// It only reads the tag; downloading and installing stay with the person, on the release page.
namespace UpdateCheck
{
	// "v1.2.3", "1.2.3" or "1.2" into three numbers. Only digits and dots pass, so a tag that passes is safe in a URL.
	inline bool Parse(const std::string& tag, int version[3])
	{
		size_t at = (!tag.empty() && (tag[0] == 'v' || tag[0] == 'V')) ? 1 : 0;
		int parts = 0;
		version[0] = version[1] = version[2] = 0;
		while (parts < 3)
		{
			size_t start = at;
			int value = 0;
			while (at < tag.size() && tag[at] >= '0' && tag[at] <= '9' && at - start < 5)
			{
				value = value * 10 + (tag[at++] - '0');
			}
			if (at == start)
			{
				return false;
			}
			version[parts++] = value;
			if (at == tag.size())
			{
				return parts >= 2;
			}
			if (tag[at++] != '.')
			{
				return false;
			}
		}
		return false;
	}

	// the first "tag_name" in GitHub's release list, which is the newest release; "" when there is none or it is no plain version
	inline std::string NewestTag(const std::string& json)
	{
		const std::string key = "\"tag_name\"";
		size_t at = json.find(key);
		if (at == std::string::npos) return "";
		at = json.find_first_not_of(" \t\r\n", at + key.size());
		if (at == std::string::npos || json[at] != ':') return "";
		at = json.find_first_not_of(" \t\r\n", at + 1);
		if (at == std::string::npos || json[at] != '"') return "";
		size_t end = json.find('"', at + 1);
		if (end == std::string::npos) return "";
		std::string tag = json.substr(at + 1, end - at - 1);
		int version[3];
		return Parse(tag, version) ? tag : "";
	}

	inline bool IsNewer(const std::string& tag, int major, int minor, int patch)
	{
		int v[3];
		if (Parse(tag, v) == false) return false;
		if (v[0] != major) return v[0] > major;
		if (v[1] != minor) return v[1] > minor;
		return v[2] > patch;
	}

	// asks GitHub on a worker thread, then posts `message` to `notify` with lParam = a new std::string* holding the
	// newest tag (the receiver deletes it), or NULL when GitHub could not be reached or answered something unexpected
	void Start(HWND notify, UINT message);
}
