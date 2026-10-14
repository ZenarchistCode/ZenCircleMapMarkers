class ZenCircleMapMarkerConfigEntry
{
	string ID = "";
	int Enabled = 1;
	vector Position = "0 0 0";
	float RadiusMeters = 1000.0;
	int ColorARGB = 0;
	int ColorRed = 255;
	int ColorGreen = 64;
	int ColorBlue = 64;
	int OutlineAlpha = 255;
	int FillAlpha = 45;
	float OutlineWidthPixels = 2.0;
	int CautionEnabled = 0;
	int EnableOverlap = 1;
	string ExpansionMarkerUID = "";
	string ExpansionMarkerName = "";
};

class ZenCircleMapMarkersConfig
{
	int Enabled = 1;
	float RenderIntervalSeconds = 0.016;
	int OutlineSegments = 96;
	float FillResolutionPixels = -1.0;
	int FillScanlines = -1;
	int RespectExpansionMarkerVisibility = 1;
	int CautionLineAlpha = 190;
	float CautionLineSpacingPixels = 18.0;
	float CautionLineWidthPixels = 2.0;
	float CautionAngleDegrees = -45.0;
	int EnableDynamicExpansionQuestCircles = 1;
	int ExpansionQuestCircleOutlineAlpha = 255;
	int ExpansionQuestCircleFillAlpha = 35;
	float ExpansionQuestCircleOutlineWidthPixels = 2.0;
	int EnableNoBuildZoneCircles = 0;
	float NoBuildZoneRefreshSeconds = 10.0;
	int NoBuildZoneCircleColorRed = 128;
	int NoBuildZoneCircleColorGreen = 0;
	int NoBuildZoneCircleColorBlue = 32;
	int NoBuildZoneCircleOutlineAlpha = 255;
	int NoBuildZoneCircleFillAlpha = 35;
	float NoBuildZoneCircleOutlineWidthPixels = 2.0;
	int DebugLogging = 0;
	ref array<ref ZenCircleMapMarkerConfigEntry> Markers;

	void ZenCircleMapMarkersConfig()
	{
		Markers = new array<ref ZenCircleMapMarkerConfigEntry>;
	}
};

class ZenCircleMapMarkerConstants
{
	static const string MOD_NAME = "ZenCircleMapMarkers";
	static const string NO_BUILD_ZONE_LINK = "nobuildzones:";
	static const string NO_BUILD_ZONE_CATEGORY_ID = "ZenCircleMapMarkers_NoBuildZones";
	static const string PROFILE_ROOT = "$profile:Zenarchist";
	static const string CONFIG_FILE = "$profile:Zenarchist\\ZenCircleMapMarkersConfig.json";
	static const float DEFAULT_RENDER_INTERVAL_SECONDS = 0.016;
	static const float LINK_REFRESH_INTERVAL_SECONDS = 0.25;
	static const int OVERLAY_SORT = 700;
	static const int MARKER_SORT = 750;
	static const int TOOLS_SORT = 900;
	static const float INTERVAL_MERGE_EPSILON_PIXELS = 0.5;
	static const float UI_EXCLUSION_PADDING_PIXELS = 1.0;
};
