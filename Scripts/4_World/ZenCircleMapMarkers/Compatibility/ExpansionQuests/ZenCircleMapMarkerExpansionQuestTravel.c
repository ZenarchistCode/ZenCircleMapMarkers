#ifdef EXPANSIONMODQUESTS
#ifdef DZ_Expansion_Navigation
modded class ExpansionQuestObjectiveTravelEvent
{
	override void CreateMarkers()
	{
		super.CreateMarkers();

		if (!ZenCircleMapMarkers_CanCreateTravelCircle())
			return;

		if (!m_Quest.GetQuestConfig().IsGroupQuest())
		{
			PlayerBase questPlayer = m_Quest.GetPlayer();
			if (questPlayer)
				ZenCircleMapMarkers_CreateTravelCircleForPlayer(questPlayer);

			return;
		}

		set<string> questMemberUIDs = m_Quest.GetPlayerUIDs();
		foreach (string questMemberUID : questMemberUIDs)
		{
			PlayerBase questGroupPlayer = PlayerBase.GetPlayerByUID(questMemberUID);
			if (questGroupPlayer)
				ZenCircleMapMarkers_CreateTravelCircleForPlayer(questGroupPlayer);
		}
	}

	override void RemoveObjectiveMarkers()
	{
		ZenCircleMapMarkers_RemoveTravelCircles();
		super.RemoveObjectiveMarkers();
	}

	override void OnGroupMemberJoined(string playerUID)
	{
		if (!ZenCircleMapMarkers_CanCreateTravelCircle())
			return;

		PlayerBase joinedGroupPlayer = PlayerBase.GetPlayerByUID(playerUID);
		if (!joinedGroupPlayer || !joinedGroupPlayer.GetIdentity())
			return;

		string joinedMarkerName = m_TravelConfig.GetMarkerName();
		m_Quest.GetQuestModule().CreateClientMarker(m_Position, joinedMarkerName, m_Quest.GetQuestConfig().GetID(), joinedGroupPlayer.GetIdentity(), m_Index);
		ZenCircleMapMarkers_CreateTravelCircleForPlayer(joinedGroupPlayer);
	}

	override void OnGroupMemberLeave(string playerUID)
	{
		if (!g_Game || !g_Game.IsServer())
			return;

		ZenCircleMapMarkerServerManager.Get().RemovePlayerCircleMarkerByUID(playerUID, ZenCircleMapMarkers_GetTravelCircleID());
	}

	protected bool ZenCircleMapMarkers_CanCreateTravelCircle()
	{
		if (!g_Game || !g_Game.IsServer())
			return false;

		ZenCircleMapMarkersConfig questCircleConfig = ZenCircleMapMarkerServerManager.Get().GetConfig();
		if (!questCircleConfig || !questCircleConfig.Enabled || !questCircleConfig.EnableDynamicExpansionQuestCircles)
			return false;

		if (!m_TravelConfig || m_TravelConfig.GetMarkerName() == string.Empty)
			return false;

		if (m_TravelConfig.GetMaxDistance() <= 0.0)
			return false;

		return true;
	}

	protected void ZenCircleMapMarkers_CreateTravelCircleForPlayer(PlayerBase targetPlayer)
	{
		if (!targetPlayer || !targetPlayer.GetIdentity())
			return;

		ZenCircleMapMarkersConfig targetCircleConfig = ZenCircleMapMarkerServerManager.Get().GetConfig();
		if (!targetCircleConfig)
			return;

		ZenCircleMapMarkerServerManager.Get().AddOrUpdatePlayerExpansionCircleMarkerByUID(targetPlayer, ZenCircleMapMarkers_GetTravelCircleID(), ZenCircleMapMarkers_GetTravelMarkerUID(), m_TravelConfig.GetMaxDistance(), targetCircleConfig.ExpansionQuestCircleOutlineAlpha, targetCircleConfig.ExpansionQuestCircleFillAlpha, targetCircleConfig.ExpansionQuestCircleOutlineWidthPixels, true);
	}

	protected void ZenCircleMapMarkers_RemoveTravelCircles()
	{
		if (!g_Game || !g_Game.IsServer() || !m_Quest)
			return;

		string removalCircleID = ZenCircleMapMarkers_GetTravelCircleID();

		if (!m_Quest.GetQuestConfig().IsGroupQuest())
		{
			string removalPlayerUID = m_Quest.GetPlayerUID();
			if (removalPlayerUID != "")
				ZenCircleMapMarkerServerManager.Get().RemovePlayerCircleMarkerByUID(removalPlayerUID, removalCircleID);

			return;
		}

		set<string> removalMemberUIDs = m_Quest.GetPlayerUIDs();
		foreach (string removalMemberUID : removalMemberUIDs)
			ZenCircleMapMarkerServerManager.Get().RemovePlayerCircleMarkerByUID(removalMemberUID, removalCircleID);
	}

	protected string ZenCircleMapMarkers_GetTravelCircleID()
	{
		return string.Format("ExpansionQuestTravel_%1_%2", m_Quest.GetQuestConfig().GetID(), m_Index);
	}

	protected string ZenCircleMapMarkers_GetTravelMarkerUID()
	{
		return string.Format("Q%1_%2", m_Quest.GetQuestConfig().GetID(), m_Index);
	}
};
#endif
#endif
