// Read the authoritative live arrays: other mods may insert directly, without an event.
modded class ZenCircleMapMarkerServerManager
{
	protected ref map<string, ref ZenCircleMapMarkerData> m_ZenNoBuildMarkers = new map<string, ref ZenCircleMapMarkerData>;

	override void Init()
	{
		if (m_Initialized || !g_Game.IsServer())
			return;

		super.Init();
		if (!m_Config.Enabled || !m_Config.EnableNoBuildZoneCircles || !ZenHasNoBuildZoneProvider())
			return;

		// A zero-radius record announces the category through the existing sync protocol.
		// It is never rendered and does not create an Expansion map/world marker.
		ZenCircleMapMarkerData category = UpsertMarkerData(ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID, "0 0 0", 0, 0, 0, 1, ZenCircleMapMarkerConstants.NO_BUILD_ZONE_LINK, false, false);
		BroadcastUpsert(category);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ZenRefreshNoBuildZones, m_Config.NoBuildZoneRefreshSeconds * 1000, true);
	}

	override void Shutdown()
	{
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ZenRefreshNoBuildZones);
		m_ZenNoBuildMarkers.Clear();
		super.Shutdown();
	}

	override protected void ValidateConfig()
	{
		super.ValidateConfig();
		m_Config.EnableNoBuildZoneCircles = Math.Clamp(m_Config.EnableNoBuildZoneCircles, 0, 1);
		m_Config.NoBuildZoneRefreshSeconds = Math.Clamp(m_Config.NoBuildZoneRefreshSeconds, 1.0, 300.0);
		m_Config.NoBuildZoneCircleColorRed = ClampByte(m_Config.NoBuildZoneCircleColorRed);
		m_Config.NoBuildZoneCircleColorGreen = ClampByte(m_Config.NoBuildZoneCircleColorGreen);
		m_Config.NoBuildZoneCircleColorBlue = ClampByte(m_Config.NoBuildZoneCircleColorBlue);
		m_Config.NoBuildZoneCircleOutlineAlpha = ClampByte(m_Config.NoBuildZoneCircleOutlineAlpha);
		m_Config.NoBuildZoneCircleFillAlpha = ClampByte(m_Config.NoBuildZoneCircleFillAlpha);
		m_Config.NoBuildZoneCircleOutlineWidthPixels = Math.Clamp(m_Config.NoBuildZoneCircleOutlineWidthPixels, 0.5, 10.0);
	}

	protected bool ZenHasNoBuildZoneProvider()
	{
		#ifdef EXPANSIONMODBASEBUILDING
		return true;
		#else
		#ifdef ZenModPack
		return true;
		#else
		return false;
		#endif
		#endif
	}

	protected void ZenRefreshNoBuildZones()
	{
		if (!g_Game.IsServer() || !m_Initialized || !m_Config.Enabled || !m_Config.EnableNoBuildZoneCircles)
			return;

		ref map<string, ref ZenCircleMapMarkerData> desired = new map<string, ref ZenCircleMapMarkerData>;
		if (!ZenCollectNoBuildZones(desired))
			return;

		foreach (string oldID, ZenCircleMapMarkerData oldMarker : m_ZenNoBuildMarkers)
		{
			if (!desired.Contains(oldID))
				RemoveCircleMarker(oldID);
		}

		foreach (string id, ZenCircleMapMarkerData marker : desired)
		{
			ZenCircleMapMarkerData previous;
			if (m_ZenNoBuildMarkers.Find(id, previous) && previous.Position == marker.Position && previous.RadiusMeters == marker.RadiusMeters && previous.OutlineColor == marker.OutlineColor && previous.FillColor == marker.FillColor && previous.OutlineWidthPixels == marker.OutlineWidthPixels)
				continue;

			ZenCircleMapMarkerData synced = UpsertMarkerData(id, marker.Position, marker.RadiusMeters, marker.OutlineColor, marker.FillColor, marker.OutlineWidthPixels, ZenCircleMapMarkerConstants.NO_BUILD_ZONE_LINK, false, true);
			BroadcastUpsert(synced);
		}

		m_ZenNoBuildMarkers = desired;
	}

	protected bool ZenCollectNoBuildZones(map<string, ref ZenCircleMapMarkerData> desired)
	{
		#ifdef EXPANSIONMODBASEBUILDING
		ExpansionBaseBuildingSettings expansionSettings = GetExpansionSettings().GetBaseBuilding(false);
		if (!expansionSettings || !expansionSettings.IsLoaded())
			return false;

		// In inverted mode these are allowed-build areas, not no-build circles.
		if (expansionSettings.ZonesAreNoBuildZones && expansionSettings.Zones)
		{
			for (int expansionIndex = 0; expansionIndex < expansionSettings.Zones.Count(); expansionIndex++)
			{
				ExpansionBuildNoBuildZone expansionZone = expansionSettings.Zones[expansionIndex];
				if (expansionZone)
					ZenCollectNoBuildCircle(desired, "Expansion_" + expansionIndex, expansionZone.Center, expansionZone.Radius);
			}
		}
		#endif

		#ifdef ZenModPack
		ZenBasebuildingConfig zenSettings = GetZenBasebuildingConfig();
		if (!zenSettings)
			return false;

		if (ZenModEnabled("ZenBasebuildingConfig") && zenSettings.NoBuildZones)
		{
			for (int zenIndex = 0; zenIndex < zenSettings.NoBuildZones.Count(); zenIndex++)
			{
				ZenNoBuildZone zenZone = zenSettings.NoBuildZones[zenIndex];
				if (zenZone)
					ZenCollectNoBuildCircle(desired, "ZenModPack_" + zenIndex, zenZone.Position, zenZone.MinimumDistance);
			}
		}
		#endif
		return true;
	}

	protected void ZenCollectNoBuildCircle(map<string, ref ZenCircleMapMarkerData> desired, string sourceID, vector position, float radius)
	{
		if (radius <= 0.0)
			return;

		ZenCircleMapMarkerData marker = new ZenCircleMapMarkerData;
		marker.ID = ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID + "_" + sourceID;
		marker.Position = position;
		marker.RadiusMeters = radius;
		marker.OutlineColor = ARGB(m_Config.NoBuildZoneCircleOutlineAlpha, m_Config.NoBuildZoneCircleColorRed, m_Config.NoBuildZoneCircleColorGreen, m_Config.NoBuildZoneCircleColorBlue);
		marker.FillColor = ARGB(m_Config.NoBuildZoneCircleFillAlpha, m_Config.NoBuildZoneCircleColorRed, m_Config.NoBuildZoneCircleColorGreen, m_Config.NoBuildZoneCircleColorBlue);
		marker.OutlineWidthPixels = m_Config.NoBuildZoneCircleOutlineWidthPixels;
		desired.Insert(marker.ID, marker);
	}
};
