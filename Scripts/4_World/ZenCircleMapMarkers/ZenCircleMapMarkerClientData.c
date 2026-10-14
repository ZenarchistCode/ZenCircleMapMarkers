class ZenCircleMapMarkerClientData
{
	protected static ref ZenCircleMapMarkerClientData s_Instance;
	protected ref array<ref ZenCircleMapMarkerData> m_Markers;
	protected bool m_Ready;
	protected bool m_RequestPending;
	protected bool m_Enabled = true;
	protected bool m_CirclesVisible = true;
	protected int m_OutlineSegments = 36;
	protected float m_FillResolutionPixels = 2.0;
	protected float m_RenderIntervalSeconds = ZenCircleMapMarkerConstants.DEFAULT_RENDER_INTERVAL_SECONDS;
	protected bool m_RespectExpansionMarkerVisibility = true;
	protected int m_CautionLineAlpha = 190;
	protected float m_CautionLineSpacingPixels = 18.0;
	protected float m_CautionLineWidthPixels = 2.0;
	protected float m_CautionAngleDegrees = -45.0;
	protected int m_Revision;

	void ZenCircleMapMarkerClientData()
	{
		m_Markers = new array<ref ZenCircleMapMarkerData>;
	}

	static ZenCircleMapMarkerClientData Get()
	{
		if (!s_Instance)
			s_Instance = new ZenCircleMapMarkerClientData;

		return s_Instance;
	}

	void Reset()
	{
		m_Markers.Clear();
		m_Ready = false;
		m_RequestPending = false;
		m_Enabled = true;
		m_CirclesVisible = true;
		m_OutlineSegments = 36;
		m_FillResolutionPixels = 2.0;
		m_RenderIntervalSeconds = ZenCircleMapMarkerConstants.DEFAULT_RENDER_INTERVAL_SECONDS;
		m_RespectExpansionMarkerVisibility = true;
		m_CautionLineAlpha = 190;
		m_CautionLineSpacingPixels = 18.0;
		m_CautionLineWidthPixels = 2.0;
		m_CautionAngleDegrees = -45.0;
		m_Revision++;
	}

	void RequestSyncIfNeeded()
	{
		if (!g_Game.IsClient())
			return;

		if (m_Ready || m_RequestPending)
			return;

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		m_RequestPending = true;
		ZenCircleMapMarkerNetwork.Get().RequestSync(player);
	}

	void BeginSync(int enabled, int outlineSegments, float fillResolutionPixels, float renderIntervalSeconds, int respectExpansionMarkerVisibility)
	{
		m_Markers.Clear();
		m_Enabled = enabled != 0;
		m_OutlineSegments = outlineSegments;
		m_FillResolutionPixels = fillResolutionPixels;
		m_RenderIntervalSeconds = renderIntervalSeconds;
		m_RespectExpansionMarkerVisibility = respectExpansionMarkerVisibility != 0;
		m_Ready = false;
		m_Revision++;
	}

	void ApplyCautionSettings(int lineAlpha, float lineSpacingPixels, float lineWidthPixels, float angleDegrees)
	{
		m_CautionLineAlpha = lineAlpha;
		m_CautionLineSpacingPixels = lineSpacingPixels;
		m_CautionLineWidthPixels = lineWidthPixels;
		m_CautionAngleDegrees = angleDegrees;
		m_Revision++;
	}

	void CompleteSync()
	{
		m_Ready = true;
		m_RequestPending = false;
		m_Revision++;
	}

	void UpsertMarker(string id, vector position, float radiusMeters, int outlineColor, int fillColor, float outlineWidthPixels, string expansionMarkerLink, int cautionEnabled, int enableOverlap)
	{
		if (id == "")
			return;

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
			return;

		markerData.Position = position;
		markerData.RadiusMeters = radiusMeters;
		markerData.OutlineColor = outlineColor;
		markerData.FillColor = fillColor;
		markerData.OutlineWidthPixels = outlineWidthPixels;
		markerData.CautionEnabled = cautionEnabled;
		markerData.EnableOverlap = enableOverlap;
		markerData.ExpansionMarkerLink = expansionMarkerLink;
		m_Revision++;
	}

	void RemoveMarker(string id)
	{
		int markerIndex = FindMarkerIndex(id);
		if (markerIndex == -1)
			return;

		m_Markers.RemoveOrdered(markerIndex);
		m_Revision++;
	}

	void ClearMarkers()
	{
		if (m_Markers.Count() == 0)
			return;

		m_Markers.Clear();
		m_Revision++;
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

	bool IsReady()
	{
		return m_Ready;
	}

	bool IsEnabled()
	{
		return m_Enabled;
	}

	bool AreCirclesVisible()
	{
		return m_CirclesVisible;
	}

	void SetCirclesVisible(bool visible)
	{
		if (m_CirclesVisible == visible)
			return;

		// Local display preference: server syncs must not overwrite it.
		m_CirclesVisible = visible;
		m_Revision++;
	}

	array<ref ZenCircleMapMarkerData> GetMarkers()
	{
		return m_Markers;
	}

	int GetOutlineSegments()
	{
		return m_OutlineSegments;
	}

	float GetFillResolutionPixels()
	{
		return m_FillResolutionPixels;
	}

	float GetRenderIntervalSeconds()
	{
		return m_RenderIntervalSeconds;
	}

	bool GetRespectExpansionMarkerVisibility()
	{
		return m_RespectExpansionMarkerVisibility;
	}

	int GetCautionLineAlpha()
	{
		return m_CautionLineAlpha;
	}

	float GetCautionLineSpacingPixels()
	{
		return m_CautionLineSpacingPixels;
	}

	float GetCautionLineWidthPixels()
	{
		return m_CautionLineWidthPixels;
	}

	float GetCautionAngleDegrees()
	{
		return m_CautionAngleDegrees;
	}

	bool HasExpansionLinkedMarkers()
	{
		foreach (ZenCircleMapMarkerData markerData : m_Markers)
		{
			if (markerData && markerData.HasExpansionLink())
				return true;
		}

		return false;
	}

	int GetRevision()
	{
		return m_Revision;
	}
};
