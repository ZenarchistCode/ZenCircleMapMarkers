modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();

		ZenCircleMapMarkerNetwork.Get().Register();
		ZenCircleMapMarkerServerManager.Get().Init();
	}

	override void OnMissionFinish()
	{
		ZenCircleMapMarkerServerManager.Get().Shutdown();

		super.OnMissionFinish();
	}
};

modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		
		ZenCircleMapMarkerClientData.Get().Reset();
		ZenCircleMapMarkerNetwork.Get().Register();
	}
};
