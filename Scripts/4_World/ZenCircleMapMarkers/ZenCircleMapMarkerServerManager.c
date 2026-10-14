class ZenCircleMapMarkerServerManager
{
	protected static ref ZenCircleMapMarkerServerManager s_Instance;
	protected ref ZenCircleMapMarkersConfig m_Config;
	protected ref array<ref ZenCircleMapMarkerData> m_Markers;
	protected ref map<string, ref array<ref ZenCircleMapMarkerData>> m_PlayerMarkers;
	protected bool m_Initialized;

	void ZenCircleMapMarkerServerManager()
	{
		m_Config = new ZenCircleMapMarkersConfig;
		m_Markers = new array<ref ZenCircleMapMarkerData>;
		m_PlayerMarkers = new map<string, ref array<ref ZenCircleMapMarkerData>>;
	}

	static ZenCircleMapMarkerServerManager Get()
	{
		if (!s_Instance)
			s_Instance = new ZenCircleMapMarkerServerManager;

		return s_Instance;
	}

	void Init()
	{
		if (m_Initialized || !g_Game.IsServer())
			return;

		m_Initialized = true;
		LoadConfig();
		ValidateConfig();
		SaveConfig();
		BuildStaticMarkers();
		Log(string.Format("Initialized with %1 configured circle markers.", m_Markers.Count()));
	}

	void Shutdown()
	{
		m_PlayerMarkers.Clear();
		m_Initialized = false;
	}

	ZenCircleMapMarkersConfig GetConfig()
	{
		return m_Config;
	}

	bool AddOrUpdateCircleMarker(string id, vector position, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		if (!g_Game.IsServer())
			return false;

		if (!m_Initialized)
			Init();

		if (!m_Config.Enabled || id == "" || radiusMeters <= 0.0)
			return false;

		int safeFillAlpha = ClampByte(fillAlpha);
		float safeOutlineWidth = Math.Clamp(outlineWidthPixels, 0.5, 10.0);
		int fillColor = BuildFillColor(outlineColor, safeFillAlpha);
		ZenCircleMapMarkerData markerData = UpsertMarkerData(id, position, radiusMeters, outlineColor, fillColor, safeOutlineWidth, "", cautionEnabled, enableOverlap);
		if (!markerData)
			return false;

		BroadcastUpsert(markerData);
		return true;
	}

	bool AddOrUpdateExpansionCircleMarkerByName(string id, string expansionMarkerName, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		if (!g_Game.IsServer())
			return false;

		if (!m_Initialized)
			Init();

		if (!m_Config.Enabled || id == "" || expansionMarkerName == "" || radiusMeters <= 0.0)
			return false;

		int safeFillAlphaByName = ClampByte(fillAlpha);
		float safeOutlineWidthByName = Math.Clamp(outlineWidthPixels, 0.5, 10.0);
		int fillColorByName = BuildFillColor(outlineColor, safeFillAlphaByName);
		ZenCircleMapMarkerData markerDataByName = UpsertMarkerData(id, "0 0 0", radiusMeters, outlineColor, fillColorByName, safeOutlineWidthByName, "name:" + expansionMarkerName, cautionEnabled, enableOverlap);
		if (!markerDataByName)
			return false;

		BroadcastUpsert(markerDataByName);
		return true;
	}

	bool AddOrUpdateExpansionCircleMarkerByUID(string id, string expansionMarkerUID, float radiusMeters, int outlineColor, int fillAlpha = 45, float outlineWidthPixels = 2.0, bool cautionEnabled = false, bool enableOverlap = true)
	{
		if (!g_Game.IsServer())
			return false;

		if (!m_Initialized)
			Init();

		if (!m_Config.Enabled || id == "" || expansionMarkerUID == "" || radiusMeters <= 0.0)
			return false;

		int safeFillAlphaByUID = ClampByte(fillAlpha);
		float safeOutlineWidthByUID = Math.Clamp(outlineWidthPixels, 0.5, 10.0);
		int fillColorByUID = BuildFillColor(outlineColor, safeFillAlphaByUID);
		ZenCircleMapMarkerData markerDataByUID = UpsertMarkerData(id, "0 0 0", radiusMeters, outlineColor, fillColorByUID, safeOutlineWidthByUID, "uid:" + expansionMarkerUID, cautionEnabled, enableOverlap);
		if (!markerDataByUID)
			return false;

		BroadcastUpsert(markerDataByUID);
		return true;
	}

	bool AddOrUpdatePlayerExpansionCircleMarkerByUID(PlayerBase player, string id, string expansionMarkerUID, float radiusMeters, int outlineAlpha = 255, int fillAlpha = 35, float outlineWidthPixels = 2.0, bool inheritLinkedMarkerColor = true, bool cautionEnabled = false, bool enableOverlap = true)
	{
		if (!g_Game.IsServer() || !player)
			return false;

		if (!m_Initialized)
			Init();

		if (!m_Config.Enabled || id == "" || expansionMarkerUID == "" || radiusMeters <= 0.0)
			return false;

		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return false;

		string playerUID = GetIdentityUID(identity);
		if (playerUID == "")
			return false;

		int safeOutlineAlpha = ClampByte(outlineAlpha);
		int safeFillAlpha = ClampByte(fillAlpha);
		float safeOutlineWidth = Math.Clamp(outlineWidthPixels, 0.5, 10.0);
		int outlineColor = ARGB(safeOutlineAlpha, 255, 255, 255);
		int fillColor = ARGB(safeFillAlpha, 255, 255, 255);
		string linkPrefix = "personaluid:";

		if (inheritLinkedMarkerColor)
			linkPrefix = "personaluidcolor:";

		array<ref ZenCircleMapMarkerData> playerMarkerList = GetOrCreatePlayerMarkerList(playerUID);
		if (!playerMarkerList)
			return false;

		ZenCircleMapMarkerData markerData = UpsertMarkerDataInArray(playerMarkerList, id, "0 0 0", radiusMeters, outlineColor, fillColor, safeOutlineWidth, linkPrefix + expansionMarkerUID, cautionEnabled, enableOverlap);
		if (!markerData)
			return false;

		ZenCircleMapMarkerNetwork.Get().SendUpsertMarker(player, identity, markerData);
		return true;
	}

	bool RemovePlayerCircleMarkerByUID(string playerUID, string id)
	{
		if (!g_Game.IsServer() || playerUID == "" || id == "")
			return false;

		if (!m_Initialized)
			Init();

		array<ref ZenCircleMapMarkerData> playerMarkerList;
		if (!m_PlayerMarkers.Find(playerUID, playerMarkerList) || !playerMarkerList)
			return false;

		int playerMarkerIndex = FindMarkerIndexInArray(playerMarkerList, id);
		if (playerMarkerIndex == -1)
			return false;

		playerMarkerList.RemoveOrdered(playerMarkerIndex);

		PlayerBase onlinePlayer = FindOnlinePlayerByUID(playerUID);
		if (onlinePlayer)
		{
			PlayerIdentity onlineIdentity = onlinePlayer.GetIdentity();
			if (onlineIdentity)
				ZenCircleMapMarkerNetwork.Get().SendRemoveMarker(onlinePlayer, onlineIdentity, id);
		}

		if (playerMarkerList.Count() == 0)
			m_PlayerMarkers.Remove(playerUID);

		return true;
	}

	bool UpdateCircleMarkerPosition(string id, vector position)
	{
		if (!g_Game.IsServer())
			return false;

		if (!m_Initialized)
			Init();

		int markerIndex = FindMarkerIndex(id);
		if (markerIndex == -1)
			return false;

		ZenCircleMapMarkerData markerData = m_Markers[markerIndex];
		if (!markerData)
			return false;

		markerData.Position = position;
		markerData.ExpansionMarkerLink = "";
		BroadcastUpsert(markerData);
		return true;
	}

	bool RemoveCircleMarker(string id)
	{
		if (!g_Game.IsServer())
			return false;

		if (!m_Initialized)
			Init();

		int markerIndex = FindMarkerIndex(id);
		if (markerIndex == -1)
			return false;

		m_Markers.RemoveOrdered(markerIndex);
		BroadcastRemove(id);
		Log("Removed circle marker " + id);
		return true;
	}

	void SyncPlayerCircleMarkers(PlayerBase player, PlayerIdentity identity)
	{
		if (!g_Game.IsServer())
			return;

		if (!m_Initialized)
			Init();

		if (!player || !identity)
			return;

		ZenCircleMapMarkerNetwork.Get().SendSyncBegin(player, identity, m_Config);
		ZenCircleMapMarkerNetwork.Get().SendSyncCautionSettings(player, identity, m_Config);

		if (m_Config.Enabled)
		{
			foreach (ZenCircleMapMarkerData markerData : m_Markers)
			{
				if (markerData)
					ZenCircleMapMarkerNetwork.Get().SendSyncMarker(player, identity, markerData);
			}

			string playerUID = GetIdentityUID(identity);
			array<ref ZenCircleMapMarkerData> playerMarkerList;
			if (playerUID != "" && m_PlayerMarkers.Find(playerUID, playerMarkerList) && playerMarkerList)
			{
				foreach (ZenCircleMapMarkerData playerMarkerData : playerMarkerList)
				{
					if (playerMarkerData)
						ZenCircleMapMarkerNetwork.Get().SendSyncMarker(player, identity, playerMarkerData);
				}
			}
		}

		ZenCircleMapMarkerNetwork.Get().SendSyncEnd(player, identity);
	}

	protected void LoadConfig()
	{
		MakeDirectory(ZenCircleMapMarkerConstants.PROFILE_ROOT);

		if (FileExist(ZenCircleMapMarkerConstants.CONFIG_FILE))
		{
			JsonFileLoader<ZenCircleMapMarkersConfig>.JsonLoadFile(ZenCircleMapMarkerConstants.CONFIG_FILE, m_Config);
			return;
		}

		float mapCenterCoordinate = g_Game.GetWorld().GetWorldSize() * 0.5;
		ZenCircleMapMarkerConfigEntry defaultMarker = new ZenCircleMapMarkerConfigEntry;
		defaultMarker.ID = "ZENARCHIST.COM";
		defaultMarker.Enabled = 1;
		defaultMarker.Position = Vector(mapCenterCoordinate, 0.0, mapCenterCoordinate);
		m_Config.Markers.Insert(defaultMarker);

		SaveConfig();
	}

	protected void SaveConfig()
	{
		MakeDirectory(ZenCircleMapMarkerConstants.PROFILE_ROOT);
		JsonFileLoader<ZenCircleMapMarkersConfig>.JsonSaveFile(ZenCircleMapMarkerConstants.CONFIG_FILE, m_Config);
	}

	protected void ValidateConfig()
	{
		if (!m_Config.Markers)
			m_Config.Markers = new array<ref ZenCircleMapMarkerConfigEntry>;

		if (m_Config.Enabled < 0)
			m_Config.Enabled = 0;
		else if (m_Config.Enabled > 1)
			m_Config.Enabled = 1;

		if (m_Config.RespectExpansionMarkerVisibility < 0)
			m_Config.RespectExpansionMarkerVisibility = 0;
		else if (m_Config.RespectExpansionMarkerVisibility > 1)
			m_Config.RespectExpansionMarkerVisibility = 1;

		m_Config.CautionLineAlpha = ClampByte(m_Config.CautionLineAlpha);
		m_Config.CautionLineSpacingPixels = Math.Clamp(m_Config.CautionLineSpacingPixels, 4.0, 200.0);
		m_Config.CautionLineWidthPixels = Math.Clamp(m_Config.CautionLineWidthPixels, 0.5, 10.0);
		m_Config.CautionAngleDegrees = Math.Clamp(m_Config.CautionAngleDegrees, -180.0, 180.0);

		if (m_Config.EnableDynamicExpansionQuestCircles < 0)
			m_Config.EnableDynamicExpansionQuestCircles = 0;
		else if (m_Config.EnableDynamicExpansionQuestCircles > 1)
			m_Config.EnableDynamicExpansionQuestCircles = 1;

		m_Config.ExpansionQuestCircleOutlineAlpha = ClampByte(m_Config.ExpansionQuestCircleOutlineAlpha);
		m_Config.ExpansionQuestCircleFillAlpha = ClampByte(m_Config.ExpansionQuestCircleFillAlpha);
		m_Config.ExpansionQuestCircleOutlineWidthPixels = Math.Clamp(m_Config.ExpansionQuestCircleOutlineWidthPixels, 0.5, 10.0);

		if (m_Config.RenderIntervalSeconds < 0.008)
			m_Config.RenderIntervalSeconds = 0.008;
		else if (m_Config.RenderIntervalSeconds > 0.1)
			m_Config.RenderIntervalSeconds = 0.1;

		if (m_Config.OutlineSegments < 12)
			m_Config.OutlineSegments = 12;
		else if (m_Config.OutlineSegments > 96)
			m_Config.OutlineSegments = 96;

		if (m_Config.FillResolutionPixels < 0.0)
		{
			m_Config.FillResolutionPixels = 2.0;
			if (m_Config.FillScanlines == 0)
				m_Config.FillResolutionPixels = 0.0;
		}
		else if (m_Config.FillResolutionPixels > 0.0 && m_Config.FillResolutionPixels < 1.0)
			m_Config.FillResolutionPixels = 1.0;
		else if (m_Config.FillResolutionPixels > 8.0)
			m_Config.FillResolutionPixels = 8.0;

		m_Config.FillScanlines = -1;

		foreach (ZenCircleMapMarkerConfigEntry markerConfig : m_Config.Markers)
		{
			if (!markerConfig)
				continue;

			if (markerConfig.Enabled < 0)
				markerConfig.Enabled = 0;
			else if (markerConfig.Enabled > 1)
				markerConfig.Enabled = 1;

			if (markerConfig.RadiusMeters < 1.0)
				markerConfig.RadiusMeters = 1.0;

			markerConfig.ColorRed = ClampByte(markerConfig.ColorRed);
			markerConfig.ColorGreen = ClampByte(markerConfig.ColorGreen);
			markerConfig.ColorBlue = ClampByte(markerConfig.ColorBlue);
			markerConfig.OutlineAlpha = ClampByte(markerConfig.OutlineAlpha);
			markerConfig.FillAlpha = ClampByte(markerConfig.FillAlpha);
			markerConfig.OutlineWidthPixels = Math.Clamp(markerConfig.OutlineWidthPixels, 0.5, 10.0);

			if (markerConfig.CautionEnabled < 0)
				markerConfig.CautionEnabled = 0;
			else if (markerConfig.CautionEnabled > 1)
				markerConfig.CautionEnabled = 1;

			if (markerConfig.EnableOverlap < 0)
				markerConfig.EnableOverlap = 0;
			else if (markerConfig.EnableOverlap > 1)
				markerConfig.EnableOverlap = 1;
		}
	}

	protected void BuildStaticMarkers()
	{
		m_Markers.Clear();

		foreach (ZenCircleMapMarkerConfigEntry markerConfig : m_Config.Markers)
		{
			if (!markerConfig || !markerConfig.Enabled || markerConfig.ID == "")
				continue;

			int markerRed = markerConfig.ColorRed;
			int markerGreen = markerConfig.ColorGreen;
			int markerBlue = markerConfig.ColorBlue;

			if (markerConfig.ColorARGB != 0)
			{
				markerRed = (markerConfig.ColorARGB >> 16) & 255;
				markerGreen = (markerConfig.ColorARGB >> 8) & 255;
				markerBlue = markerConfig.ColorARGB & 255;
			}

			int outlineColor = ARGB(markerConfig.OutlineAlpha, markerRed, markerGreen, markerBlue);
			int fillColor = ARGB(markerConfig.FillAlpha, markerRed, markerGreen, markerBlue);
			string expansionMarkerLink = "";

			if (markerConfig.ExpansionMarkerUID != "")
				expansionMarkerLink = "uid:" + markerConfig.ExpansionMarkerUID;
			else if (markerConfig.ExpansionMarkerName != "")
				expansionMarkerLink = "name:" + markerConfig.ExpansionMarkerName;

			UpsertMarkerData(markerConfig.ID, markerConfig.Position, markerConfig.RadiusMeters, outlineColor, fillColor, markerConfig.OutlineWidthPixels, expansionMarkerLink, markerConfig.CautionEnabled != 0, markerConfig.EnableOverlap != 0);
		}
	}

	protected ZenCircleMapMarkerData UpsertMarkerData(string id, vector position, float radiusMeters, int outlineColor, int fillColor, float outlineWidthPixels, string expansionMarkerLink, bool cautionEnabled, bool enableOverlap)
	{
		int markerIndex = FindMarkerIndex(id);
		ZenCircleMapMarkerData markerData;

		if (markerIndex == -1)
		{
			markerData = new ZenCircleMapMarkerData;
			markerData.ID = id;
			m_Markers.Insert(markerData);
		}
		else
		{
			markerData = m_Markers[markerIndex];
		}

		if (!markerData)
			return null;

		markerData.Position = position;
		markerData.RadiusMeters = radiusMeters;
		markerData.OutlineColor = outlineColor;
		markerData.FillColor = fillColor;
		markerData.OutlineWidthPixels = outlineWidthPixels;
		markerData.CautionEnabled = cautionEnabled;
		markerData.EnableOverlap = enableOverlap;
		markerData.ExpansionMarkerLink = expansionMarkerLink;
		return markerData;
	}

	protected array<ref ZenCircleMapMarkerData> GetOrCreatePlayerMarkerList(string playerUID)
	{
		array<ref ZenCircleMapMarkerData> playerMarkerList;
		if (m_PlayerMarkers.Find(playerUID, playerMarkerList) && playerMarkerList)
			return playerMarkerList;

		playerMarkerList = new array<ref ZenCircleMapMarkerData>;
		m_PlayerMarkers.Insert(playerUID, playerMarkerList);
		return playerMarkerList;
	}

	protected ZenCircleMapMarkerData UpsertMarkerDataInArray(array<ref ZenCircleMapMarkerData> markerList, string id, vector position, float radiusMeters, int outlineColor, int fillColor, float outlineWidthPixels, string expansionMarkerLink, bool cautionEnabled, bool enableOverlap)
	{
		if (!markerList)
			return null;

		int markerIndex = FindMarkerIndexInArray(markerList, id);
		ZenCircleMapMarkerData markerData;

		if (markerIndex == -1)
		{
			markerData = new ZenCircleMapMarkerData;
			markerData.ID = id;
			markerList.Insert(markerData);
		}
		else
		{
			markerData = markerList[markerIndex];
		}

		if (!markerData)
			return null;

		markerData.Position = position;
		markerData.RadiusMeters = radiusMeters;
		markerData.OutlineColor = outlineColor;
		markerData.FillColor = fillColor;
		markerData.OutlineWidthPixels = outlineWidthPixels;
		markerData.CautionEnabled = cautionEnabled;
		markerData.EnableOverlap = enableOverlap;
		markerData.ExpansionMarkerLink = expansionMarkerLink;
		return markerData;
	}

	protected int FindMarkerIndexInArray(array<ref ZenCircleMapMarkerData> markerList, string id)
	{
		if (!markerList)
			return -1;

		for (int markerArrayIndex = 0; markerArrayIndex < markerList.Count(); markerArrayIndex++)
		{
			ZenCircleMapMarkerData markerArrayData = markerList[markerArrayIndex];
			if (markerArrayData && markerArrayData.ID == id)
				return markerArrayIndex;
		}

		return -1;
	}

	protected string GetIdentityUID(PlayerIdentity identity)
	{
		if (!identity)
			return "";

		string identityUID = identity.GetId();
		if (identityUID == "")
			identityUID = identity.GetPlainId();

		return identityUID;
	}

	protected PlayerBase FindOnlinePlayerByUID(string playerUID)
	{
		if (playerUID == "")
			return null;

		ref array<Man> onlinePlayers = new array<Man>;
		g_Game.GetPlayers(onlinePlayers);

		foreach (Man onlineMan : onlinePlayers)
		{
			PlayerBase onlinePlayerCandidate = PlayerBase.Cast(onlineMan);
			if (!onlinePlayerCandidate)
				continue;

			PlayerIdentity onlinePlayerIdentity = onlinePlayerCandidate.GetIdentity();
			if (!onlinePlayerIdentity)
				continue;

			string onlinePlayerUID = GetIdentityUID(onlinePlayerIdentity);
			if (onlinePlayerUID == playerUID)
				return onlinePlayerCandidate;
		}

		return null;
	}

	protected int FindMarkerIndex(string id)
	{
		for (int markerIndex = 0; markerIndex < m_Markers.Count(); markerIndex++)
		{
			ZenCircleMapMarkerData markerData = m_Markers[markerIndex];
			if (markerData && markerData.ID == id)
				return markerIndex;
		}

		return -1;
	}

	protected int ClampByte(int value)
	{
		if (value < 0)
			return 0;

		if (value > 255)
			return 255;

		return value;
	}

	protected int BuildFillColor(int outlineColor, int fillAlpha)
	{
		int red = (outlineColor >> 16) & 255;
		int green = (outlineColor >> 8) & 255;
		int blue = outlineColor & 255;
		return ARGB(fillAlpha, red, green, blue);
	}

	protected void BroadcastUpsert(ZenCircleMapMarkerData markerData)
	{
		if (!markerData)
			return;

		ref array<Man> players = new array<Man>;
		g_Game.GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (!player)
				continue;

			PlayerIdentity identity = player.GetIdentity();
			if (!identity)
				continue;

			ZenCircleMapMarkerNetwork.Get().SendUpsertMarker(player, identity, markerData);
		}
	}

	protected void BroadcastRemove(string id)
	{
		ref array<Man> players = new array<Man>;
		g_Game.GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (!player)
				continue;

			PlayerIdentity identity = player.GetIdentity();
			if (!identity)
				continue;

			ZenCircleMapMarkerNetwork.Get().SendRemoveMarker(player, identity, id);
		}
	}

	protected void Log(string message)
	{
		if (!m_Config.DebugLogging)
			return;

		Print("[ZenCircleMapMarkers] " + message);
	}
};
