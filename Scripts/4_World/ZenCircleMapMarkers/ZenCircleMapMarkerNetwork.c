class ZenCircleMapMarkerNetwork
{
	protected static ref ZenCircleMapMarkerNetwork s_Instance;
	protected bool m_Registered;

	static ZenCircleMapMarkerNetwork Get()
	{
		if (!s_Instance)
			s_Instance = new ZenCircleMapMarkerNetwork;

		return s_Instance;
	}

	void Register()
	{
		if (m_Registered)
			return;

		m_Registered = true;
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_RequestSync", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncBegin", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncCautionSettings", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncMarker", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncEnd", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_UpsertMarker", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_RemoveMarker", this, SingleplayerExecutionType.Client);
	}

	void RequestSync(PlayerBase player)
	{
		if (!g_Game.IsClient() || !player)
			return;

		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_RequestSync", new Param1<int>(1), true, null, player);
	}

	void SendSyncBegin(PlayerBase player, PlayerIdentity identity, ZenCircleMapMarkersConfig config)
	{
		if (!g_Game.IsServer() || !player || !identity || !config)
			return;

		Param5<int, int, float, float, int> data = new Param5<int, int, float, float, int>(config.Enabled, config.OutlineSegments, config.FillResolutionPixels, config.RenderIntervalSeconds, config.RespectExpansionMarkerVisibility);
		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncBegin", data, true, identity, player);
	}

	void SendSyncCautionSettings(PlayerBase player, PlayerIdentity identity, ZenCircleMapMarkersConfig config)
	{
		if (!g_Game.IsServer() || !player || !identity || !config)
			return;

		Param4<int, float, float, float> data = new Param4<int, float, float, float>(config.CautionLineAlpha, config.CautionLineSpacingPixels, config.CautionLineWidthPixels, config.CautionAngleDegrees);
		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncCautionSettings", data, true, identity, player);
	}

	void SendSyncMarker(PlayerBase player, PlayerIdentity identity, ZenCircleMapMarkerData markerData)
	{
		if (!g_Game.IsServer() || !player || !identity || !markerData)
			return;

		Param9<string, vector, float, int, int, float, string, int, int> data = new Param9<string, vector, float, int, int, float, string, int, int>(markerData.ID, markerData.Position, markerData.RadiusMeters, markerData.OutlineColor, markerData.FillColor, markerData.OutlineWidthPixels, markerData.ExpansionMarkerLink, markerData.CautionEnabled, markerData.EnableOverlap);
		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncMarker", data, true, identity, player);
	}

	void SendSyncEnd(PlayerBase player, PlayerIdentity identity)
	{
		if (!g_Game.IsServer() || !player || !identity)
			return;

		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_SyncEnd", new Param1<int>(1), true, identity, player);
	}

	void SendUpsertMarker(PlayerBase player, PlayerIdentity identity, ZenCircleMapMarkerData markerData)
	{
		if (!g_Game.IsServer() || !player || !identity || !markerData)
			return;

		Param9<string, vector, float, int, int, float, string, int, int> data = new Param9<string, vector, float, int, int, float, string, int, int>(markerData.ID, markerData.Position, markerData.RadiusMeters, markerData.OutlineColor, markerData.FillColor, markerData.OutlineWidthPixels, markerData.ExpansionMarkerLink, markerData.CautionEnabled, markerData.EnableOverlap);
		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_UpsertMarker", data, true, identity, player);
	}

	void SendRemoveMarker(PlayerBase player, PlayerIdentity identity, string id)
	{
		if (!g_Game.IsServer() || !player || !identity || id == "")
			return;

		GetRPCManager().SendRPC(ZenCircleMapMarkerConstants.MOD_NAME, "RPC_RemoveMarker", new Param1<string>(id), true, identity, player);
	}

	void RPC_RequestSync(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
			return;

		Param1<int> request;
		if (!ctx.Read(request))
			return;

		PlayerBase player = PlayerBase.Cast(target);
		if (!player || !player.GetIdentity())
			return;

		if (player.GetIdentity().GetId() != sender.GetId())
			return;

		ZenCircleMapMarkerServerManager.Get().SyncPlayerCircleMarkers(player, sender);
	}

	void RPC_SyncBegin(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param5<int, int, float, float, int> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().BeginSync(data.param1, data.param2, data.param3, data.param4, data.param5);
	}

	void RPC_SyncCautionSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param4<int, float, float, float> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().ApplyCautionSettings(data.param1, data.param2, data.param3, data.param4);
	}

	void RPC_SyncMarker(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param9<string, vector, float, int, int, float, string, int, int> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().UpsertMarker(data.param1, data.param2, data.param3, data.param4, data.param5, data.param6, data.param7, data.param8, data.param9);
	}

	void RPC_SyncEnd(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<int> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().CompleteSync();
	}

	void RPC_UpsertMarker(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param9<string, vector, float, int, int, float, string, int, int> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().UpsertMarker(data.param1, data.param2, data.param3, data.param4, data.param5, data.param6, data.param7, data.param8, data.param9);
	}

	void RPC_RemoveMarker(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<string> data;
		if (!ctx.Read(data))
			return;

		ZenCircleMapMarkerClientData.Get().RemoveMarker(data.param1);
	}
};
