class CfgPatches
{
	class ZenCircleMapMarkers
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Data",
			"DZ_Scripts",
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class ZenCircleMapMarkers
	{
		dir = "ZenCircleMapMarkers";
		name = "ZenCircleMapMarkers";
		author = "Zenarchist";
		type = "mod";
		dependencies[] =
		{
			"Game",
			"World",
			"Mission"
		};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] =
				{
					"ZenCircleMapMarkers/Scripts/3_Game"
				};
			};

			class worldScriptModule
			{
				value = "";
				files[] =
				{
					"ZenCircleMapMarkers/Scripts/4_World"
				};
			};

			class missionScriptModule
			{
				value = "";
				files[] =
				{
					"ZenCircleMapMarkers/Scripts/5_Mission"
				};
			};
		};
	};
};
