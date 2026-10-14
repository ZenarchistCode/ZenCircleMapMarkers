class ZenCircleMapMarkers
{
	static bool AddCircleMarker(string id, vector position, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		return ZenCircleMapMarkerServerManager.Get().AddOrUpdateCircleMarker(id, position, radiusMeters, outlineColor, fillAlpha, outlineWidthPixels, cautionEnabled, enableOverlap);
	}

	static bool AddExpansionCircleMarkerByName(string id, string expansionMarkerName, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		return ZenCircleMapMarkerServerManager.Get().AddOrUpdateExpansionCircleMarkerByName(id, expansionMarkerName, radiusMeters, outlineColor, fillAlpha, outlineWidthPixels, cautionEnabled, enableOverlap);
	}

	static bool AddExpansionCircleMarkerByUID(string id, string expansionMarkerUID, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		return ZenCircleMapMarkerServerManager.Get().AddOrUpdateExpansionCircleMarkerByUID(id, expansionMarkerUID, radiusMeters, outlineColor, fillAlpha, outlineWidthPixels, cautionEnabled, enableOverlap);
	}

	static bool UpdateCircleMarkerPosition(string id, vector position)
	{
		return ZenCircleMapMarkerServerManager.Get().UpdateCircleMarkerPosition(id, position);
	}

	static bool RemoveCircleMarker(string id)
	{
		return ZenCircleMapMarkerServerManager.Get().RemoveCircleMarker(id);
	}
};
