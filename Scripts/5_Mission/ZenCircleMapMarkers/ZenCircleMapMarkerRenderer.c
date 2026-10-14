class ZenCircleMapMarkerRenderEntry
{
	ZenCircleMapMarkerData MarkerData;
	vector WorldPosition;
	float CenterX;
	float CenterY;
	float RadiusX;
	float RadiusY;
	int OutlineColor;
	int FillColor;
};

class ZenCircleMapMarkerRenderGroup
{
	ref array<ref ZenCircleMapMarkerRenderEntry> Members;

	void ZenCircleMapMarkerRenderGroup()
	{
		Members = new array<ref ZenCircleMapMarkerRenderEntry>;
	}
};

class ZenCircleMapMarkerInterval
{
	float Start;
	float End;

	void ZenCircleMapMarkerInterval(float intervalStart, float intervalEnd)
	{
		Start = intervalStart;
		End = intervalEnd;
	}
};

class ZenCircleMapMarkerRenderer
{
	protected MapWidget m_MapWidget;
	protected Widget m_MenuRoot;
	protected CanvasWidget m_Canvas;
	protected float m_LastScale = -1.0;
	protected vector m_LastMapPosition;
	protected int m_LastRevision = -1;
	protected float m_RenderAccumulator;
	protected float m_LinkRefreshAccumulator;
	protected bool m_Shown;
	protected bool m_CreateFailureLogged;
	protected bool m_HasUIExclusion;
	protected float m_UIExclusionLeft;
	protected float m_UIExclusionTop;
	protected float m_UIExclusionRight;
	protected float m_UIExclusionBottom;
	protected ref array<ref ZenCircleMapMarkerRenderEntry> m_ActiveCutoutEntries;

	void ZenCircleMapMarkerRenderer(MapWidget mapWidget, Widget menuRoot)
	{
		m_MapWidget = mapWidget;
		m_MenuRoot = menuRoot;
	}

	void ~ZenCircleMapMarkerRenderer()
	{
		if (m_Canvas)
		{
			m_Canvas.Clear();
			m_Canvas.Unlink();
		}
	}

	void Show()
	{
		m_Shown = true;
		m_LastScale = -1.0;
		m_LastRevision = -1;
		m_RenderAccumulator = ZenCircleMapMarkerConstants.DEFAULT_RENDER_INTERVAL_SECONDS;
		m_LinkRefreshAccumulator = ZenCircleMapMarkerConstants.LINK_REFRESH_INTERVAL_SECONDS;
		EnsureCanvas();
		ZenCircleMapMarkerClientData.Get().RequestSyncIfNeeded();
	}

	void Hide()
	{
		m_Shown = false;

		if (m_Canvas)
		{
			m_Canvas.Clear();
			m_Canvas.Show(false);
		}
	}

	void Update(float timeslice)
	{
		if (!m_Shown || !m_MapWidget || !m_MenuRoot)
			return;

		EnsureCanvas();
		if (!m_Canvas)
			return;

		ZenCircleMapMarkerClientData clientData = ZenCircleMapMarkerClientData.Get();
		clientData.RequestSyncIfNeeded();

		if (!clientData.IsEnabled() || !clientData.AreCirclesVisible())
		{
			m_Canvas.Clear();
			m_Canvas.Show(false);
			return;
		}

		if (!clientData.IsReady())
			return;

		m_RenderAccumulator += timeslice;
		m_LinkRefreshAccumulator += timeslice;

		float currentScale = m_MapWidget.GetScale();
		vector currentMapPosition = m_MapWidget.GetMapPos();
		int currentRevision = clientData.GetRevision();
		bool scaleChanged = Math.AbsFloat(currentScale - m_LastScale) > 0.00001;
		bool positionChanged = Math.AbsFloat(currentMapPosition[0] - m_LastMapPosition[0]) > 0.01 || Math.AbsFloat(currentMapPosition[2] - m_LastMapPosition[2]) > 0.01;
		bool revisionChanged = currentRevision != m_LastRevision;
		bool linkedRefreshDue = clientData.HasExpansionLinkedMarkers() && m_LinkRefreshAccumulator >= ZenCircleMapMarkerConstants.LINK_REFRESH_INTERVAL_SECONDS;

		if (!scaleChanged && !positionChanged && !revisionChanged && !linkedRefreshDue)
			return;

		float renderInterval = clientData.GetRenderIntervalSeconds();
		if (positionChanged && !scaleChanged && !revisionChanged && !linkedRefreshDue && m_RenderAccumulator < renderInterval)
			return;

		if (m_RenderAccumulator >= renderInterval)
		{
			m_RenderAccumulator -= renderInterval;

			if (m_RenderAccumulator > renderInterval)
				m_RenderAccumulator = renderInterval;
		}
		else
		{
			m_RenderAccumulator = 0.0;
		}

		m_LastScale = currentScale;
		m_LastMapPosition = currentMapPosition;
		m_LastRevision = currentRevision;
		m_LinkRefreshAccumulator = 0.0;
		Refresh(clientData);
	}

	protected void EnsureCanvas()
	{
		if (m_Canvas || !m_MenuRoot)
			return;

		Widget canvasWidget = g_Game.GetWorkspace().CreateWidgets("ZenCircleMapMarkers/GUI/layouts/zen_circle_map_canvas.layout", m_MenuRoot);
		m_Canvas = CanvasWidget.Cast(canvasWidget);

		if (!m_Canvas)
		{
			if (!m_CreateFailureLogged)
			{
				m_CreateFailureLogged = true;
				Print("[ZenCircleMapMarkers] ERROR: Failed to create map circle CanvasWidget.");
			}

			return;
		}

		m_Canvas.SetSort(ZenCircleMapMarkerConstants.OVERLAY_SORT);
		m_Canvas.Show(false);
	}

	protected void Refresh(ZenCircleMapMarkerClientData clientData)
	{
		if (!clientData || !m_Canvas)
			return;

		float mapScreenX;
		float mapScreenY;
		float mapScreenWidth;
		float mapScreenHeight;
		m_MapWidget.GetScreenPos(mapScreenX, mapScreenY);
		m_MapWidget.GetScreenSize(mapScreenWidth, mapScreenHeight);

		if (mapScreenWidth <= 0.0 || mapScreenHeight <= 0.0)
		{
			m_Canvas.Clear();
			m_Canvas.Show(false);
			return;
		}

		m_Canvas.SetScreenPos(mapScreenX, mapScreenY);
		m_Canvas.SetScreenSize(mapScreenWidth, mapScreenHeight);
		m_Canvas.SetSort(ZenCircleMapMarkerConstants.OVERLAY_SORT);
		m_Canvas.Show(true);
		m_Canvas.Clear();
		UpdateUIExclusionRect(mapScreenX, mapScreenY);

		array<ref ZenCircleMapMarkerData> circleMarkers = clientData.GetMarkers();
		if (!circleMarkers)
			return;

		ref array<ref ZenCircleMapMarkerRenderEntry> renderEntries = new array<ref ZenCircleMapMarkerRenderEntry>;

		foreach (ZenCircleMapMarkerData circleMarker : circleMarkers)
		{
			if (!circleMarker || circleMarker.RadiusMeters <= 0.0)
				continue;

			vector markerWorldPosition;
			int markerOutlineColor;
			int markerFillColor;
			if (!ResolveMarkerRenderData(circleMarker, clientData.GetRespectExpansionMarkerVisibility(), markerWorldPosition, markerOutlineColor, markerFillColor))
				continue;

			vector markerCenterScreen = m_MapWidget.MapToScreen(markerWorldPosition);
			vector markerRadiusWorldX = markerWorldPosition;
			markerRadiusWorldX[0] = markerRadiusWorldX[0] + circleMarker.RadiusMeters;
			vector markerRadiusScreenX = m_MapWidget.MapToScreen(markerRadiusWorldX);
			vector markerRadiusWorldZ = markerWorldPosition;
			markerRadiusWorldZ[2] = markerRadiusWorldZ[2] + circleMarker.RadiusMeters;
			vector markerRadiusScreenZ = m_MapWidget.MapToScreen(markerRadiusWorldZ);

			float markerRadiusPixelsX = Math.AbsFloat(markerRadiusScreenX[0] - markerCenterScreen[0]);
			float markerRadiusPixelsY = Math.AbsFloat(markerRadiusScreenZ[1] - markerCenterScreen[1]);
			if (markerRadiusPixelsX <= 0.5 || markerRadiusPixelsY <= 0.5)
				continue;

			float markerLocalX = markerCenterScreen[0] - mapScreenX;
			float markerLocalY = markerCenterScreen[1] - mapScreenY;
			if ((markerLocalX + markerRadiusPixelsX) < 0.0 || (markerLocalX - markerRadiusPixelsX) > mapScreenWidth || (markerLocalY + markerRadiusPixelsY) < 0.0 || (markerLocalY - markerRadiusPixelsY) > mapScreenHeight)
				continue;

			ZenCircleMapMarkerRenderEntry renderEntry = new ZenCircleMapMarkerRenderEntry;
			renderEntry.MarkerData = circleMarker;
			renderEntry.WorldPosition = markerWorldPosition;
			renderEntry.CenterX = markerLocalX;
			renderEntry.CenterY = markerLocalY;
			renderEntry.RadiusX = markerRadiusPixelsX;
			renderEntry.RadiusY = markerRadiusPixelsY;
			renderEntry.OutlineColor = markerOutlineColor;
			renderEntry.FillColor = markerFillColor;
			renderEntries.Insert(renderEntry);
		}

		ref array<ref ZenCircleMapMarkerRenderGroup> renderGroups = BuildRenderGroups(renderEntries);
		SortRenderGroupsByRadius(renderGroups);
		for (int renderGroupIndex = 0; renderGroupIndex < renderGroups.Count(); renderGroupIndex++)
		{
			ZenCircleMapMarkerRenderGroup renderGroup = renderGroups[renderGroupIndex];
			if (!renderGroup || !renderGroup.Members || renderGroup.Members.Count() == 0)
				continue;

			m_ActiveCutoutEntries = BuildGroupCutouts(renderGroups, renderGroupIndex);

			if (renderGroup.Members.Count() == 1)
			{
				DrawSingleMarker(renderGroup.Members[0], clientData, mapScreenWidth, mapScreenHeight);
				continue;
			}

			DrawMergedMarkerGroup(renderGroup, clientData, mapScreenWidth, mapScreenHeight);
		}

		m_ActiveCutoutEntries = null;
	}

	protected void UpdateUIExclusionRect(float canvasScreenX, float canvasScreenY)
	{
		m_HasUIExclusion = false;

		if (!m_MenuRoot)
			return;

		Widget exclusionWidget = m_MenuRoot.FindAnyWidget("ZMF_Map_Tools_Container");
		if (!exclusionWidget || !exclusionWidget.IsVisible())
			exclusionWidget = m_MenuRoot.FindAnyWidget("Map_Tools_Container");

		if (!exclusionWidget || !exclusionWidget.IsVisible())
			return;

		float exclusionScreenX;
		float exclusionScreenY;
		float exclusionScreenWidth;
		float exclusionScreenHeight;
		exclusionWidget.GetScreenPos(exclusionScreenX, exclusionScreenY);
		exclusionWidget.GetScreenSize(exclusionScreenWidth, exclusionScreenHeight);

		if (exclusionScreenWidth <= 0.0 || exclusionScreenHeight <= 0.0)
			return;

		float exclusionPadding = ZenCircleMapMarkerConstants.UI_EXCLUSION_PADDING_PIXELS;
		m_UIExclusionLeft = (exclusionScreenX - canvasScreenX) - exclusionPadding;
		m_UIExclusionTop = (exclusionScreenY - canvasScreenY) - exclusionPadding;
		m_UIExclusionRight = (exclusionScreenX - canvasScreenX) + exclusionScreenWidth + exclusionPadding;
		m_UIExclusionBottom = (exclusionScreenY - canvasScreenY) + exclusionScreenHeight + exclusionPadding;
		m_HasUIExclusion = true;
	}

	protected void DrawCanvasLine(float startX, float startY, float endX, float endY, float lineWidth, int color)
	{
		if (!m_Canvas)
			return;

		if (!m_ActiveCutoutEntries || m_ActiveCutoutEntries.Count() == 0)
		{
			DrawUnmaskedCanvasLine(startX, startY, endX, endY, lineWidth, color);
			return;
		}

		ref array<ref ZenCircleMapMarkerInterval> cutoutIntervals;
		float cutoutPadding = lineWidth * 0.5;

		foreach (ZenCircleMapMarkerRenderEntry cutoutEntry : m_ActiveCutoutEntries)
		{
			float cutoutStartT;
			float cutoutEndT;
			if (GetSegmentEllipseCoverageInterval(startX, startY, endX, endY, cutoutEntry, cutoutStartT, cutoutEndT, cutoutPadding))
			{
				if (!cutoutIntervals)
					cutoutIntervals = new array<ref ZenCircleMapMarkerInterval>;

				cutoutIntervals.Insert(new ZenCircleMapMarkerInterval(cutoutStartT, cutoutEndT));
			}
		}

		if (!cutoutIntervals)
		{
			DrawUnmaskedCanvasLine(startX, startY, endX, endY, lineWidth, color);
			return;
		}

		SortIntervals(cutoutIntervals);
		float visibleStartT = 0.0;
		float lineDeltaX = endX - startX;
		float lineDeltaY = endY - startY;

		foreach (ZenCircleMapMarkerInterval cutoutInterval : cutoutIntervals)
		{
			if (cutoutInterval.End <= visibleStartT)
				continue;

			if (cutoutInterval.Start > visibleStartT + 0.00001)
			{
				float visibleEndX = startX + (lineDeltaX * cutoutInterval.Start);
				float visibleEndY = startY + (lineDeltaY * cutoutInterval.Start);
				float visibleStartX = startX + (lineDeltaX * visibleStartT);
				float visibleStartY = startY + (lineDeltaY * visibleStartT);
				DrawUnmaskedCanvasLine(visibleStartX, visibleStartY, visibleEndX, visibleEndY, lineWidth, color);
			}

			visibleStartT = cutoutInterval.End;
			if (visibleStartT >= 0.99999)
				return;
		}

		if (visibleStartT < 0.99999)
		{
			float remainingStartX = startX + (lineDeltaX * visibleStartT);
			float remainingStartY = startY + (lineDeltaY * visibleStartT);
			DrawUnmaskedCanvasLine(remainingStartX, remainingStartY, endX, endY, lineWidth, color);
		}
	}

	protected void DrawUnmaskedCanvasLine(float startX, float startY, float endX, float endY, float lineWidth, int color)
	{
		if (!m_Canvas)
			return;

		if (!m_HasUIExclusion)
		{
			m_Canvas.DrawLine(startX, startY, endX, endY, lineWidth, color);
			return;
		}

		float lineExclusionPadding = lineWidth * 0.5;
		float lineExclusionLeft = m_UIExclusionLeft - lineExclusionPadding;
		float lineExclusionTop = m_UIExclusionTop - lineExclusionPadding;
		float lineExclusionRight = m_UIExclusionRight + lineExclusionPadding;
		float lineExclusionBottom = m_UIExclusionBottom + lineExclusionPadding;
		float insideStartT;
		float insideEndT;
		if (!GetSegmentRectangleIntersectionInterval(startX, startY, endX, endY, lineExclusionLeft, lineExclusionTop, lineExclusionRight, lineExclusionBottom, insideStartT, insideEndT))
		{
			m_Canvas.DrawLine(startX, startY, endX, endY, lineWidth, color);
			return;
		}

		float lineDeltaX = endX - startX;
		float lineDeltaY = endY - startY;

		if (insideStartT > 0.0001)
		{
			float firstEndX = startX + (lineDeltaX * insideStartT);
			float firstEndY = startY + (lineDeltaY * insideStartT);
			m_Canvas.DrawLine(startX, startY, firstEndX, firstEndY, lineWidth, color);
		}

		if (insideEndT < 0.9999)
		{
			float secondStartX = startX + (lineDeltaX * insideEndT);
			float secondStartY = startY + (lineDeltaY * insideEndT);
			m_Canvas.DrawLine(secondStartX, secondStartY, endX, endY, lineWidth, color);
		}
	}

	protected bool GetSegmentRectangleIntersectionInterval(float startX, float startY, float endX, float endY, float rectangleLeft, float rectangleTop, float rectangleRight, float rectangleBottom, out float intervalStart, out float intervalEnd)
	{
		intervalStart = 0.0;
		intervalEnd = 1.0;

		float rectangleDeltaX = endX - startX;
		if (Math.AbsFloat(rectangleDeltaX) < 0.00001)
		{
			if (startX < rectangleLeft || startX > rectangleRight)
				return false;
		}
		else
		{
			float rectangleXFirst = (rectangleLeft - startX) / rectangleDeltaX;
			float rectangleXSecond = (rectangleRight - startX) / rectangleDeltaX;
			if (rectangleXFirst > rectangleXSecond)
			{
				float rectangleXSwap = rectangleXFirst;
				rectangleXFirst = rectangleXSecond;
				rectangleXSecond = rectangleXSwap;
			}

			if (rectangleXFirst > intervalStart)
				intervalStart = rectangleXFirst;

			if (rectangleXSecond < intervalEnd)
				intervalEnd = rectangleXSecond;

			if (intervalEnd < intervalStart)
				return false;
		}

		float rectangleDeltaY = endY - startY;
		if (Math.AbsFloat(rectangleDeltaY) < 0.00001)
		{
			if (startY < rectangleTop || startY > rectangleBottom)
				return false;
		}
		else
		{
			float rectangleYFirst = (rectangleTop - startY) / rectangleDeltaY;
			float rectangleYSecond = (rectangleBottom - startY) / rectangleDeltaY;
			if (rectangleYFirst > rectangleYSecond)
			{
				float rectangleYSwap = rectangleYFirst;
				rectangleYFirst = rectangleYSecond;
				rectangleYSecond = rectangleYSwap;
			}

			if (rectangleYFirst > intervalStart)
				intervalStart = rectangleYFirst;

			if (rectangleYSecond < intervalEnd)
				intervalEnd = rectangleYSecond;

			if (intervalEnd < intervalStart)
				return false;
		}

		if (intervalEnd < 0.0 || intervalStart > 1.0)
			return false;

		if (intervalStart < 0.0)
			intervalStart = 0.0;

		if (intervalEnd > 1.0)
			intervalEnd = 1.0;

		return intervalEnd >= intervalStart;
	}

	protected array<ref ZenCircleMapMarkerRenderGroup> BuildRenderGroups(array<ref ZenCircleMapMarkerRenderEntry> renderEntries)
	{
		ref array<ref ZenCircleMapMarkerRenderGroup> renderGroups = new array<ref ZenCircleMapMarkerRenderGroup>;
		if (!renderEntries || renderEntries.Count() == 0)
			return renderGroups;

		ref set<int> assignedIndices = new set<int>;

		for (int entryIndex = 0; entryIndex < renderEntries.Count(); entryIndex++)
		{
			if (assignedIndices.Find(entryIndex) != -1)
				continue;

			ZenCircleMapMarkerRenderEntry baseEntry = renderEntries[entryIndex];
			if (!baseEntry)
				continue;

			ZenCircleMapMarkerRenderGroup newGroup = new ZenCircleMapMarkerRenderGroup;
			newGroup.Members.Insert(baseEntry);
			assignedIndices.Insert(entryIndex);

			if (baseEntry.MarkerData && baseEntry.MarkerData.EnableOverlap)
			{
				int groupScanIndex = 0;
				while (groupScanIndex < newGroup.Members.Count())
				{
					ZenCircleMapMarkerRenderEntry sourceEntry = newGroup.Members[groupScanIndex];

					for (int candidateIndex = 0; candidateIndex < renderEntries.Count(); candidateIndex++)
					{
						if (assignedIndices.Find(candidateIndex) != -1)
							continue;

						ZenCircleMapMarkerRenderEntry candidateEntry = renderEntries[candidateIndex];
						if (!candidateEntry || !candidateEntry.MarkerData || !candidateEntry.MarkerData.EnableOverlap)
							continue;

						if (!AreMergeStylesCompatible(baseEntry, candidateEntry))
							continue;

						if (!DoWorldCirclesOverlap(sourceEntry, candidateEntry))
							continue;

						newGroup.Members.Insert(candidateEntry);
						assignedIndices.Insert(candidateIndex);
					}

					groupScanIndex++;
				}
			}

			renderGroups.Insert(newGroup);
		}

		return renderGroups;
	}

	protected float GetRenderGroupRadius(ZenCircleMapMarkerRenderGroup renderGroup)
	{
		float maxRadius = 0.0;
		if (!renderGroup || !renderGroup.Members)
			return maxRadius;

		foreach (ZenCircleMapMarkerRenderEntry radiusEntry : renderGroup.Members)
		{
			if (radiusEntry && radiusEntry.MarkerData && radiusEntry.MarkerData.RadiusMeters > maxRadius)
				maxRadius = radiusEntry.MarkerData.RadiusMeters;
		}

		return maxRadius;
	}

	protected void SortRenderGroupsByRadius(array<ref ZenCircleMapMarkerRenderGroup> renderGroups)
	{
		if (!renderGroups)
			return;

		for (int sortGroupIndex = 1; sortGroupIndex < renderGroups.Count(); sortGroupIndex++)
		{
			ZenCircleMapMarkerRenderGroup sortedGroup = renderGroups[sortGroupIndex];
			float sortedRadius = GetRenderGroupRadius(sortedGroup);
			int insertIndex = sortGroupIndex;
			while (insertIndex > 0 && GetRenderGroupRadius(renderGroups[insertIndex - 1]) < sortedRadius)
			{
				renderGroups.Set(insertIndex, renderGroups[insertIndex - 1]);
				insertIndex--;
			}

			renderGroups.Set(insertIndex, sortedGroup);
		}
	}

	protected array<ref ZenCircleMapMarkerRenderEntry> BuildGroupCutouts(array<ref ZenCircleMapMarkerRenderGroup> renderGroups, int currentGroupIndex)
	{
		ref array<ref ZenCircleMapMarkerRenderEntry> cutouts = new array<ref ZenCircleMapMarkerRenderEntry>;
		if (!renderGroups || currentGroupIndex < 0 || currentGroupIndex >= renderGroups.Count())
			return cutouts;

		ZenCircleMapMarkerRenderGroup currentGroup = renderGroups[currentGroupIndex];
		if (!currentGroup || !currentGroup.Members || currentGroup.Members.Count() == 0)
			return cutouts;

		ZenCircleMapMarkerRenderEntry currentStyle = currentGroup.Members[0];
		for (int cutoutGroupIndex = currentGroupIndex + 1; cutoutGroupIndex < renderGroups.Count(); cutoutGroupIndex++)
		{
			ZenCircleMapMarkerRenderGroup cutoutGroup = renderGroups[cutoutGroupIndex];
			if (!cutoutGroup || !cutoutGroup.Members || cutoutGroup.Members.Count() == 0 || !HaveDifferentRenderColors(currentStyle, cutoutGroup.Members[0]))
				continue;

			foreach (ZenCircleMapMarkerRenderEntry candidateCutout : cutoutGroup.Members)
			{
				foreach (ZenCircleMapMarkerRenderEntry currentMember : currentGroup.Members)
				{
					if (DoWorldCirclesOverlap(currentMember, candidateCutout))
					{
						cutouts.Insert(candidateCutout);
						break;
					}
				}
			}
		}

		return cutouts;
	}

	protected bool HaveDifferentRenderColors(ZenCircleMapMarkerRenderEntry firstEntry, ZenCircleMapMarkerRenderEntry secondEntry)
	{
		if (!firstEntry || !secondEntry)
			return false;

		return (firstEntry.OutlineColor & 0x00FFFFFF) != (secondEntry.OutlineColor & 0x00FFFFFF) || (firstEntry.FillColor & 0x00FFFFFF) != (secondEntry.FillColor & 0x00FFFFFF);
	}

	protected bool AreMergeStylesCompatible(ZenCircleMapMarkerRenderEntry firstEntry, ZenCircleMapMarkerRenderEntry secondEntry)
	{
		if (!firstEntry || !secondEntry || !firstEntry.MarkerData || !secondEntry.MarkerData)
			return false;

		if (firstEntry.OutlineColor != secondEntry.OutlineColor)
			return false;

		if (firstEntry.FillColor != secondEntry.FillColor)
			return false;

		if (firstEntry.MarkerData.CautionEnabled != secondEntry.MarkerData.CautionEnabled)
			return false;

		if (Math.AbsFloat(firstEntry.MarkerData.OutlineWidthPixels - secondEntry.MarkerData.OutlineWidthPixels) > 0.01)
			return false;

		return true;
	}

	protected bool DoWorldCirclesOverlap(ZenCircleMapMarkerRenderEntry firstEntry, ZenCircleMapMarkerRenderEntry secondEntry)
	{
		if (!firstEntry || !secondEntry || !firstEntry.MarkerData || !secondEntry.MarkerData)
			return false;

		float worldDeltaX = firstEntry.WorldPosition[0] - secondEntry.WorldPosition[0];
		float worldDeltaZ = firstEntry.WorldPosition[2] - secondEntry.WorldPosition[2];
		float combinedRadius = firstEntry.MarkerData.RadiusMeters + secondEntry.MarkerData.RadiusMeters;
		return (worldDeltaX * worldDeltaX) + (worldDeltaZ * worldDeltaZ) <= (combinedRadius * combinedRadius);
	}

	protected void DrawSingleMarker(ZenCircleMapMarkerRenderEntry renderEntry, ZenCircleMapMarkerClientData clientData, float canvasWidth, float canvasHeight)
	{
		if (!renderEntry || !renderEntry.MarkerData || !clientData)
			return;

		DrawFilledEllipse(renderEntry.CenterX, renderEntry.CenterY, renderEntry.RadiusX, renderEntry.RadiusY, renderEntry.FillColor, clientData.GetFillResolutionPixels(), canvasWidth, canvasHeight);

		if (renderEntry.MarkerData.CautionEnabled)
		{
			int singleCautionRed = (renderEntry.OutlineColor >> 16) & 255;
			int singleCautionGreen = (renderEntry.OutlineColor >> 8) & 255;
			int singleCautionBlue = renderEntry.OutlineColor & 255;
			int singleCautionColor = ARGB(clientData.GetCautionLineAlpha(), singleCautionRed, singleCautionGreen, singleCautionBlue);
			DrawCautionHatch(renderEntry.CenterX, renderEntry.CenterY, renderEntry.RadiusX, renderEntry.RadiusY, singleCautionColor, clientData.GetCautionLineSpacingPixels(), clientData.GetCautionLineWidthPixels(), clientData.GetCautionAngleDegrees());
		}

		DrawEllipseOutline(renderEntry.CenterX, renderEntry.CenterY, renderEntry.RadiusX, renderEntry.RadiusY, renderEntry.OutlineColor, renderEntry.MarkerData.OutlineWidthPixels, clientData.GetOutlineSegments());
	}

	protected void DrawMergedMarkerGroup(ZenCircleMapMarkerRenderGroup renderGroup, ZenCircleMapMarkerClientData clientData, float canvasWidth, float canvasHeight)
	{
		if (!renderGroup || !renderGroup.Members || renderGroup.Members.Count() < 2 || !clientData)
			return;

		ZenCircleMapMarkerRenderEntry styleEntry = renderGroup.Members[0];
		if (!styleEntry || !styleEntry.MarkerData)
			return;

		DrawMergedFill(renderGroup, styleEntry.FillColor, clientData.GetFillResolutionPixels(), canvasWidth, canvasHeight);

		if (styleEntry.MarkerData.CautionEnabled)
		{
			int mergedCautionRed = (styleEntry.OutlineColor >> 16) & 255;
			int mergedCautionGreen = (styleEntry.OutlineColor >> 8) & 255;
			int mergedCautionBlue = styleEntry.OutlineColor & 255;
			int mergedCautionColor = ARGB(clientData.GetCautionLineAlpha(), mergedCautionRed, mergedCautionGreen, mergedCautionBlue);
			DrawMergedCautionHatch(renderGroup, mergedCautionColor, clientData.GetCautionLineSpacingPixels(), clientData.GetCautionLineWidthPixels(), clientData.GetCautionAngleDegrees());
		}

		DrawMergedOutline(renderGroup, styleEntry.OutlineColor, styleEntry.MarkerData.OutlineWidthPixels, clientData.GetOutlineSegments());
	}

	protected void DrawMergedFill(ZenCircleMapMarkerRenderGroup renderGroup, int color, float resolutionPixels, float canvasWidth, float canvasHeight)
	{
		int mergedFillAlpha = (color >> 24) & 255;
		if (mergedFillAlpha <= 0 || resolutionPixels <= 0.0 || !renderGroup || !renderGroup.Members)
			return;

		float mergedStripHeight = resolutionPixels;
		if (mergedStripHeight < 1.0)
			mergedStripHeight = 1.0;

		float mergedMinY = 0.0;
		float mergedMaxY = 0.0;
		bool mergedBoundsSet = false;

		foreach (ZenCircleMapMarkerRenderEntry fillEntry : renderGroup.Members)
		{
			if (!fillEntry || fillEntry.RadiusX <= 0.0 || fillEntry.RadiusY <= 0.0)
				continue;

			float fillEntryMinY = fillEntry.CenterY - fillEntry.RadiusY;
			float fillEntryMaxY = fillEntry.CenterY + fillEntry.RadiusY;
			if (!mergedBoundsSet)
			{
				mergedMinY = fillEntryMinY;
				mergedMaxY = fillEntryMaxY;
				mergedBoundsSet = true;
			}
			else
			{
				if (fillEntryMinY < mergedMinY)
					mergedMinY = fillEntryMinY;

				if (fillEntryMaxY > mergedMaxY)
					mergedMaxY = fillEntryMaxY;

			}
		}

		if (!mergedBoundsSet)
			return;

		float mergedVisibleMinY = mergedMinY;
		if (mergedVisibleMinY < 0.0)
			mergedVisibleMinY = 0.0;

		float mergedVisibleMaxY = mergedMaxY;
		if (mergedVisibleMaxY > canvasHeight)
			mergedVisibleMaxY = canvasHeight;

		if (mergedVisibleMaxY <= mergedVisibleMinY)
			return;

		float mergedFirstStripIndex = Math.Floor((mergedVisibleMinY - mergedMinY) / mergedStripHeight);
		float mergedStripStartY = mergedMinY + (mergedFirstStripIndex * mergedStripHeight);
		while (mergedStripStartY < mergedVisibleMaxY)
		{
			float mergedStripEndY = mergedStripStartY + mergedStripHeight;
			if (mergedStripEndY > mergedMaxY)
				mergedStripEndY = mergedMaxY;

			float mergedDrawY = (mergedStripStartY + mergedStripEndY) * 0.5;
			float mergedLineWidth = mergedStripEndY - mergedStripStartY;
			if (mergedLineWidth > 0.0 && mergedDrawY + (mergedLineWidth * 0.5) > 0.0 && mergedDrawY - (mergedLineWidth * 0.5) < canvasHeight)
			{
				ref array<ref ZenCircleMapMarkerInterval> fillIntervals = new array<ref ZenCircleMapMarkerInterval>;

				foreach (ZenCircleMapMarkerRenderEntry fillIntervalEntry : renderGroup.Members)
				{
					if (!fillIntervalEntry || fillIntervalEntry.RadiusX <= 0.0 || fillIntervalEntry.RadiusY <= 0.0)
						continue;

					float fillDeltaY = mergedDrawY - fillIntervalEntry.CenterY;
					float fillNormalizedY = fillDeltaY / fillIntervalEntry.RadiusY;
					float fillNormalizedYSquared = fillNormalizedY * fillNormalizedY;
					if (fillNormalizedYSquared > 1.0)
						continue;

					float fillHalfWidth = fillIntervalEntry.RadiusX * Math.Sqrt(1.0 - fillNormalizedYSquared);
					fillIntervals.Insert(new ZenCircleMapMarkerInterval(fillIntervalEntry.CenterX - fillHalfWidth, fillIntervalEntry.CenterX + fillHalfWidth));
				}

				DrawMergedHorizontalIntervals(fillIntervals, mergedDrawY, mergedLineWidth, color, canvasWidth);
			}

			mergedStripStartY = mergedStripStartY + mergedStripHeight;
		}
	}

	protected void DrawMergedHorizontalIntervals(array<ref ZenCircleMapMarkerInterval> intervals, float drawY, float lineWidth, int color, float canvasWidth)
	{
		if (!intervals || intervals.Count() == 0)
			return;

		SortIntervals(intervals);

		ZenCircleMapMarkerInterval firstInterval = intervals[0];
		if (!firstInterval)
			return;

		float activeStart = firstInterval.Start;
		float activeEnd = firstInterval.End;

		for (int mergeIndex = 1; mergeIndex < intervals.Count(); mergeIndex++)
		{
			ZenCircleMapMarkerInterval mergeInterval = intervals[mergeIndex];
			if (!mergeInterval)
				continue;

			if (mergeInterval.Start <= activeEnd + ZenCircleMapMarkerConstants.INTERVAL_MERGE_EPSILON_PIXELS)
			{
				if (mergeInterval.End > activeEnd)
					activeEnd = mergeInterval.End;

				continue;
			}

			DrawClippedHorizontalLine(activeStart, activeEnd, drawY, lineWidth, color, canvasWidth);
			activeStart = mergeInterval.Start;
			activeEnd = mergeInterval.End;
		}

		DrawClippedHorizontalLine(activeStart, activeEnd, drawY, lineWidth, color, canvasWidth);
	}

	protected void DrawClippedHorizontalLine(float startX, float endX, float drawY, float lineWidth, int color, float canvasWidth)
	{
		float clippedStartX = startX;
		float clippedEndX = endX;

		if (clippedStartX < 0.0)
			clippedStartX = 0.0;

		if (clippedEndX > canvasWidth)
			clippedEndX = canvasWidth;

		if (clippedEndX <= clippedStartX)
			return;

		DrawCanvasLine(clippedStartX, drawY, clippedEndX, drawY, lineWidth, color);
	}

	protected void DrawMergedCautionHatch(ZenCircleMapMarkerRenderGroup renderGroup, int color, float spacingPixels, float lineWidthPixels, float angleDegrees)
	{
		int mergedCautionAlpha = (color >> 24) & 255;
		if (mergedCautionAlpha <= 0 || spacingPixels <= 0.0 || lineWidthPixels <= 0.0 || !renderGroup || !renderGroup.Members)
			return;

		float mergedAngleRadians = angleDegrees * Math.DEG2RAD;
		float mergedDirectionX = Math.Cos(mergedAngleRadians);
		float mergedDirectionY = Math.Sin(mergedAngleRadians);
		float mergedNormalX = -mergedDirectionY;
		float mergedNormalY = mergedDirectionX;
		float mergedMinOffset = 0.0;
		float mergedMaxOffset = 0.0;
		bool mergedOffsetSet = false;

		foreach (ZenCircleMapMarkerRenderEntry cautionBoundsEntry : renderGroup.Members)
		{
			if (!cautionBoundsEntry || cautionBoundsEntry.RadiusX <= 0.0 || cautionBoundsEntry.RadiusY <= 0.0)
				continue;

			float cautionCenterOffset = (cautionBoundsEntry.CenterX * mergedNormalX) + (cautionBoundsEntry.CenterY * mergedNormalY);
			float cautionNormalExtent = Math.Sqrt((cautionBoundsEntry.RadiusX * mergedNormalX * cautionBoundsEntry.RadiusX * mergedNormalX) + (cautionBoundsEntry.RadiusY * mergedNormalY * cautionBoundsEntry.RadiusY * mergedNormalY));
			float cautionEntryMinOffset = cautionCenterOffset - cautionNormalExtent;
			float cautionEntryMaxOffset = cautionCenterOffset + cautionNormalExtent;

			if (!mergedOffsetSet)
			{
				mergedMinOffset = cautionEntryMinOffset;
				mergedMaxOffset = cautionEntryMaxOffset;
				mergedOffsetSet = true;
			}
			else
			{
				if (cautionEntryMinOffset < mergedMinOffset)
					mergedMinOffset = cautionEntryMinOffset;

				if (cautionEntryMaxOffset > mergedMaxOffset)
					mergedMaxOffset = cautionEntryMaxOffset;
			}
		}

		if (!mergedOffsetSet)
			return;

		float firstCautionOffset = Math.Floor(mergedMinOffset / spacingPixels) * spacingPixels;
		float currentCautionOffset = firstCautionOffset;
		while (currentCautionOffset <= mergedMaxOffset)
		{
			ref array<ref ZenCircleMapMarkerInterval> cautionIntervals = new array<ref ZenCircleMapMarkerInterval>;
			float cautionBaseX = mergedNormalX * currentCautionOffset;
			float cautionBaseY = mergedNormalY * currentCautionOffset;

			foreach (ZenCircleMapMarkerRenderEntry cautionEntry : renderGroup.Members)
			{
				if (!cautionEntry || cautionEntry.RadiusX <= 0.0 || cautionEntry.RadiusY <= 0.0)
					continue;

				float cautionRelativeX = cautionBaseX - cautionEntry.CenterX;
				float cautionRelativeY = cautionBaseY - cautionEntry.CenterY;
				float cautionInverseRadiusXSquared = 1.0 / (cautionEntry.RadiusX * cautionEntry.RadiusX);
				float cautionInverseRadiusYSquared = 1.0 / (cautionEntry.RadiusY * cautionEntry.RadiusY);
				float cautionQuadraticA = (mergedDirectionX * mergedDirectionX * cautionInverseRadiusXSquared) + (mergedDirectionY * mergedDirectionY * cautionInverseRadiusYSquared);
				float cautionQuadraticB = 2.0 * ((cautionRelativeX * mergedDirectionX * cautionInverseRadiusXSquared) + (cautionRelativeY * mergedDirectionY * cautionInverseRadiusYSquared));
				float cautionQuadraticC = (cautionRelativeX * cautionRelativeX * cautionInverseRadiusXSquared) + (cautionRelativeY * cautionRelativeY * cautionInverseRadiusYSquared) - 1.0;
				float cautionDiscriminant = (cautionQuadraticB * cautionQuadraticB) - (4.0 * cautionQuadraticA * cautionQuadraticC);

				if (cautionDiscriminant < 0.0 || cautionQuadraticA <= 0.0)
					continue;

				float cautionRootValue = Math.Sqrt(cautionDiscriminant);
				float cautionFirstDistance = (-cautionQuadraticB - cautionRootValue) / (2.0 * cautionQuadraticA);
				float cautionSecondDistance = (-cautionQuadraticB + cautionRootValue) / (2.0 * cautionQuadraticA);
				cautionIntervals.Insert(new ZenCircleMapMarkerInterval(cautionFirstDistance, cautionSecondDistance));
			}

			DrawMergedDirectionalIntervals(cautionIntervals, cautionBaseX, cautionBaseY, mergedDirectionX, mergedDirectionY, lineWidthPixels, color);
			currentCautionOffset = currentCautionOffset + spacingPixels;
		}
	}

	protected void DrawMergedDirectionalIntervals(array<ref ZenCircleMapMarkerInterval> intervals, float baseX, float baseY, float directionX, float directionY, float lineWidth, int color)
	{
		if (!intervals || intervals.Count() == 0)
			return;

		SortIntervals(intervals);

		ZenCircleMapMarkerInterval firstDirectionalInterval = intervals[0];
		if (!firstDirectionalInterval)
			return;

		float directionalStart = firstDirectionalInterval.Start;
		float directionalEnd = firstDirectionalInterval.End;

		for (int directionalIndex = 1; directionalIndex < intervals.Count(); directionalIndex++)
		{
			ZenCircleMapMarkerInterval directionalInterval = intervals[directionalIndex];
			if (!directionalInterval)
				continue;

			if (directionalInterval.Start <= directionalEnd + ZenCircleMapMarkerConstants.INTERVAL_MERGE_EPSILON_PIXELS)
			{
				if (directionalInterval.End > directionalEnd)
					directionalEnd = directionalInterval.End;

				continue;
			}

			DrawDirectionalLine(baseX, baseY, directionX, directionY, directionalStart, directionalEnd, lineWidth, color);
			directionalStart = directionalInterval.Start;
			directionalEnd = directionalInterval.End;
		}

		DrawDirectionalLine(baseX, baseY, directionX, directionY, directionalStart, directionalEnd, lineWidth, color);
	}

	protected void DrawDirectionalLine(float baseX, float baseY, float directionX, float directionY, float startDistance, float endDistance, float lineWidth, int color)
	{
		float directionalStartX = baseX + (directionX * startDistance);
		float directionalStartY = baseY + (directionY * startDistance);
		float directionalEndX = baseX + (directionX * endDistance);
		float directionalEndY = baseY + (directionY * endDistance);
		DrawCanvasLine(directionalStartX, directionalStartY, directionalEndX, directionalEndY, lineWidth, color);
	}

	protected void DrawMergedOutline(ZenCircleMapMarkerRenderGroup renderGroup, int color, float lineWidth, int segments)
	{
		int mergedOutlineAlpha = (color >> 24) & 255;
		if (mergedOutlineAlpha <= 0 || segments < 3 || lineWidth <= 0.0 || !renderGroup || !renderGroup.Members)
			return;

		float mergedAngleStepDegrees = 360.0 / segments;

		for (int outlineMemberIndex = 0; outlineMemberIndex < renderGroup.Members.Count(); outlineMemberIndex++)
		{
			ZenCircleMapMarkerRenderEntry outlineEntry = renderGroup.Members[outlineMemberIndex];
			if (!outlineEntry || outlineEntry.RadiusX <= 0.0 || outlineEntry.RadiusY <= 0.0)
				continue;

			float mergedPreviousX = outlineEntry.CenterX + outlineEntry.RadiusX;
			float mergedPreviousY = outlineEntry.CenterY;

			for (int mergedOutlineIndex = 1; mergedOutlineIndex <= segments; mergedOutlineIndex++)
			{
				float mergedAngleRadians = (mergedOutlineIndex * mergedAngleStepDegrees) * Math.DEG2RAD;
				float mergedNextX = outlineEntry.CenterX + (Math.Cos(mergedAngleRadians) * outlineEntry.RadiusX);
				float mergedNextY = outlineEntry.CenterY + (Math.Sin(mergedAngleRadians) * outlineEntry.RadiusY);

				DrawMergedOutlineSegment(renderGroup, outlineMemberIndex, mergedPreviousX, mergedPreviousY, mergedNextX, mergedNextY, lineWidth, color);

				mergedPreviousX = mergedNextX;
				mergedPreviousY = mergedNextY;
			}
		}
	}

	protected void DrawMergedOutlineSegment(ZenCircleMapMarkerRenderGroup renderGroup, int currentMemberIndex, float segmentStartX, float segmentStartY, float segmentEndX, float segmentEndY, float lineWidth, int color)
	{
		if (!renderGroup || !renderGroup.Members)
			return;

		ZenCircleMapMarkerRenderEntry currentOutlineEntry = renderGroup.Members[currentMemberIndex];
		if (!currentOutlineEntry)
			return;

		ref array<ref ZenCircleMapMarkerInterval> coveredIntervals = new array<ref ZenCircleMapMarkerInterval>;

		for (int coverOutlineIndex = 0; coverOutlineIndex < renderGroup.Members.Count(); coverOutlineIndex++)
		{
			if (coverOutlineIndex == currentMemberIndex)
				continue;

			ZenCircleMapMarkerRenderEntry coverOutlineEntry = renderGroup.Members[coverOutlineIndex];
			if (!coverOutlineEntry || coverOutlineEntry.RadiusX <= 0.0 || coverOutlineEntry.RadiusY <= 0.0)
				continue;

			float duplicateOutlineCenterDeltaX = currentOutlineEntry.CenterX - coverOutlineEntry.CenterX;
			float duplicateOutlineCenterDeltaY = currentOutlineEntry.CenterY - coverOutlineEntry.CenterY;
			bool duplicateOutlineCircle = Math.AbsFloat(duplicateOutlineCenterDeltaX) <= 0.01 && Math.AbsFloat(duplicateOutlineCenterDeltaY) <= 0.01 && Math.AbsFloat(currentOutlineEntry.RadiusX - coverOutlineEntry.RadiusX) <= 0.01 && Math.AbsFloat(currentOutlineEntry.RadiusY - coverOutlineEntry.RadiusY) <= 0.01;
			if (duplicateOutlineCircle)
			{
				if (coverOutlineIndex < currentMemberIndex)
				{
					coveredIntervals.Insert(new ZenCircleMapMarkerInterval(0.0, 1.0));
					break;
				}

				continue;
			}

			float coveredSegmentStartT;
			float coveredSegmentEndT;
			if (!GetSegmentEllipseCoverageInterval(segmentStartX, segmentStartY, segmentEndX, segmentEndY, coverOutlineEntry, coveredSegmentStartT, coveredSegmentEndT))
				continue;

			if ((coveredSegmentEndT - coveredSegmentStartT) <= 0.00001)
				continue;

			coveredIntervals.Insert(new ZenCircleMapMarkerInterval(coveredSegmentStartT, coveredSegmentEndT));
		}

		if (coveredIntervals.Count() == 0)
		{
			DrawCanvasLine(segmentStartX, segmentStartY, segmentEndX, segmentEndY, lineWidth, color);
			return;
		}

		SortIntervals(coveredIntervals);

		float outlineSegmentDeltaX = segmentEndX - segmentStartX;
		float outlineSegmentDeltaY = segmentEndY - segmentStartY;
		float uncoveredStartT = 0.0;

		foreach (ZenCircleMapMarkerInterval coveredOutlineInterval : coveredIntervals)
		{
			if (!coveredOutlineInterval)
				continue;

			float coveredOutlineStartT = coveredOutlineInterval.Start;
			float coveredOutlineEndT = coveredOutlineInterval.End;

			if (coveredOutlineStartT < 0.0)
				coveredOutlineStartT = 0.0;

			if (coveredOutlineEndT > 1.0)
				coveredOutlineEndT = 1.0;

			if (coveredOutlineEndT <= uncoveredStartT)
				continue;

			if (coveredOutlineStartT > uncoveredStartT + 0.00001)
			{
				float exposedStartX = segmentStartX + (outlineSegmentDeltaX * uncoveredStartT);
				float exposedStartY = segmentStartY + (outlineSegmentDeltaY * uncoveredStartT);
				float exposedEndX = segmentStartX + (outlineSegmentDeltaX * coveredOutlineStartT);
				float exposedEndY = segmentStartY + (outlineSegmentDeltaY * coveredOutlineStartT);
				DrawCanvasLine(exposedStartX, exposedStartY, exposedEndX, exposedEndY, lineWidth, color);
			}

			if (coveredOutlineEndT > uncoveredStartT)
				uncoveredStartT = coveredOutlineEndT;

			if (uncoveredStartT >= 0.99999)
				return;
		}

		if (uncoveredStartT < 0.99999)
		{
			float remainingStartX = segmentStartX + (outlineSegmentDeltaX * uncoveredStartT);
			float remainingStartY = segmentStartY + (outlineSegmentDeltaY * uncoveredStartT);
			DrawCanvasLine(remainingStartX, remainingStartY, segmentEndX, segmentEndY, lineWidth, color);
		}
	}

	protected bool GetSegmentEllipseCoverageInterval(float segmentStartX, float segmentStartY, float segmentEndX, float segmentEndY, ZenCircleMapMarkerRenderEntry coverEntry, out float coverageStartT, out float coverageEndT, float paddingPixels = 0.0)
	{
		coverageStartT = 0.0;
		coverageEndT = 0.0;

		if (!coverEntry || coverEntry.RadiusX <= 0.0 || coverEntry.RadiusY <= 0.0)
			return false;

		float ellipseSegmentDeltaX = segmentEndX - segmentStartX;
		float ellipseSegmentDeltaY = segmentEndY - segmentStartY;
		float ellipseRelativeStartX = segmentStartX - coverEntry.CenterX;
		float ellipseRelativeStartY = segmentStartY - coverEntry.CenterY;
		float paddedRadiusX = coverEntry.RadiusX + paddingPixels;
		float paddedRadiusY = coverEntry.RadiusY + paddingPixels;
		if ((segmentStartX < coverEntry.CenterX - paddedRadiusX && segmentEndX < coverEntry.CenterX - paddedRadiusX) || (segmentStartX > coverEntry.CenterX + paddedRadiusX && segmentEndX > coverEntry.CenterX + paddedRadiusX) || (segmentStartY < coverEntry.CenterY - paddedRadiusY && segmentEndY < coverEntry.CenterY - paddedRadiusY) || (segmentStartY > coverEntry.CenterY + paddedRadiusY && segmentEndY > coverEntry.CenterY + paddedRadiusY))
			return false;

		float ellipseInverseRadiusXSquared = 1.0 / (paddedRadiusX * paddedRadiusX);
		float ellipseInverseRadiusYSquared = 1.0 / (paddedRadiusY * paddedRadiusY);
		float ellipseQuadraticA = (ellipseSegmentDeltaX * ellipseSegmentDeltaX * ellipseInverseRadiusXSquared) + (ellipseSegmentDeltaY * ellipseSegmentDeltaY * ellipseInverseRadiusYSquared);
		float ellipseQuadraticB = 2.0 * ((ellipseRelativeStartX * ellipseSegmentDeltaX * ellipseInverseRadiusXSquared) + (ellipseRelativeStartY * ellipseSegmentDeltaY * ellipseInverseRadiusYSquared));
		float ellipseQuadraticC = (ellipseRelativeStartX * ellipseRelativeStartX * ellipseInverseRadiusXSquared) + (ellipseRelativeStartY * ellipseRelativeStartY * ellipseInverseRadiusYSquared) - 1.0;

		if (ellipseQuadraticA <= 0.0000001)
		{
			if (ellipseQuadraticC < 0.0)
			{
				coverageStartT = 0.0;
				coverageEndT = 1.0;
				return true;
			}

			return false;
		}

		float ellipseDiscriminant = (ellipseQuadraticB * ellipseQuadraticB) - (4.0 * ellipseQuadraticA * ellipseQuadraticC);
		if (ellipseDiscriminant < 0.0)
		{
			if (ellipseQuadraticC < 0.0)
			{
				coverageStartT = 0.0;
				coverageEndT = 1.0;
				return true;
			}

			return false;
		}

		float ellipseRootValue = Math.Sqrt(ellipseDiscriminant);
		float ellipseFirstT = (-ellipseQuadraticB - ellipseRootValue) / (2.0 * ellipseQuadraticA);
		float ellipseSecondT = (-ellipseQuadraticB + ellipseRootValue) / (2.0 * ellipseQuadraticA);
		if (ellipseFirstT > ellipseSecondT)
		{
			float ellipseSwapT = ellipseFirstT;
			ellipseFirstT = ellipseSecondT;
			ellipseSecondT = ellipseSwapT;
		}

		if (ellipseSecondT <= 0.0 || ellipseFirstT >= 1.0)
			return false;

		coverageStartT = ellipseFirstT;
		coverageEndT = ellipseSecondT;

		if (coverageStartT < 0.0)
			coverageStartT = 0.0;

		if (coverageEndT > 1.0)
			coverageEndT = 1.0;

		return coverageEndT > coverageStartT;
	}

	protected void SortIntervals(array<ref ZenCircleMapMarkerInterval> intervals)
	{
		if (!intervals || intervals.Count() < 2)
			return;

		for (int sortIndex = 0; sortIndex < intervals.Count() - 1; sortIndex++)
		{
			int lowestIndex = sortIndex;
			ZenCircleMapMarkerInterval lowestInterval = intervals[lowestIndex];

			for (int searchIndex = sortIndex + 1; searchIndex < intervals.Count(); searchIndex++)
			{
				ZenCircleMapMarkerInterval searchInterval = intervals[searchIndex];
				if (!searchInterval)
					continue;

				if (!lowestInterval || searchInterval.Start < lowestInterval.Start)
				{
					lowestIndex = searchIndex;
					lowestInterval = searchInterval;
				}
			}

			if (lowestIndex == sortIndex)
				continue;

			ZenCircleMapMarkerInterval swapInterval = intervals[sortIndex];
			intervals.Set(sortIndex, intervals[lowestIndex]);
			intervals.Set(lowestIndex, swapInterval);
		}
	}

	protected void DrawFilledEllipse(float centerX, float centerY, float radiusX, float radiusY, int color, float resolutionPixels, float canvasWidth, float canvasHeight)
	{
		int fillAlpha = (color >> 24) & 255;
		if (fillAlpha <= 0 || resolutionPixels <= 0.0 || radiusX <= 0.0 || radiusY <= 0.0)
			return;

		float stripHeight = resolutionPixels;
		if (stripHeight < 1.0)
			stripHeight = 1.0;

		float circleMinY = centerY - radiusY;
		float circleMaxY = centerY + radiusY;
		float visibleMinY = circleMinY;
		if (visibleMinY < 0.0)
			visibleMinY = 0.0;

		float visibleMaxY = circleMaxY;
		if (visibleMaxY > canvasHeight)
			visibleMaxY = canvasHeight;

		if (visibleMaxY <= visibleMinY)
			return;

		float firstStripIndex = Math.Floor((visibleMinY - circleMinY) / stripHeight);
		float stripStartY = circleMinY + (firstStripIndex * stripHeight);
		while (stripStartY < visibleMaxY)
		{
			float stripEndY = stripStartY + stripHeight;
			if (stripEndY > circleMaxY)
				stripEndY = circleMaxY;

			float drawY = (stripStartY + stripEndY) * 0.5;
			float lineWidth = stripEndY - stripStartY;
			if (lineWidth <= 0.0)
				break;

			float normalizedY = (drawY - centerY) / radiusY;
			float squaredWidth = 1.0 - (normalizedY * normalizedY);
			if (squaredWidth < 0.0)
				squaredWidth = 0.0;

			float halfWidth = radiusX * Math.Sqrt(squaredWidth);

			float drawStartX = centerX - halfWidth;
			float drawEndX = centerX + halfWidth;

			if (drawStartX < 0.0)
				drawStartX = 0.0;

			if (drawEndX > canvasWidth)
				drawEndX = canvasWidth;

			if (drawEndX > drawStartX && drawY + (lineWidth * 0.5) > 0.0 && drawY - (lineWidth * 0.5) < canvasHeight)
				DrawCanvasLine(drawStartX, drawY, drawEndX, drawY, lineWidth, color);

			stripStartY = stripStartY + stripHeight;
		}
	}

	protected void DrawCautionHatch(float centerX, float centerY, float radiusX, float radiusY, int color, float spacingPixels, float lineWidthPixels, float angleDegrees)
	{
		int cautionAlpha = (color >> 24) & 255;
		if (cautionAlpha <= 0 || spacingPixels <= 0.0 || lineWidthPixels <= 0.0 || radiusX <= 0.0 || radiusY <= 0.0)
			return;

		float angleRadians = angleDegrees * Math.DEG2RAD;
		float directionX = Math.Cos(angleRadians);
		float directionY = Math.Sin(angleRadians);
		float normalX = -directionY;
		float normalY = directionX;
		float inverseRadiusXSquared = 1.0 / (radiusX * radiusX);
		float inverseRadiusYSquared = 1.0 / (radiusY * radiusY);
		float maxOffset = Math.Sqrt((radiusX * normalX * radiusX * normalX) + (radiusY * normalY * radiusY * normalY));

		for (float hatchOffset = -maxOffset; hatchOffset <= maxOffset; hatchOffset = hatchOffset + spacingPixels)
		{
			float offsetX = hatchOffset * normalX;
			float offsetY = hatchOffset * normalY;
			float quadraticA = (directionX * directionX * inverseRadiusXSquared) + (directionY * directionY * inverseRadiusYSquared);
			float quadraticB = 2.0 * ((offsetX * directionX * inverseRadiusXSquared) + (offsetY * directionY * inverseRadiusYSquared));
			float quadraticC = (offsetX * offsetX * inverseRadiusXSquared) + (offsetY * offsetY * inverseRadiusYSquared) - 1.0;
			float discriminant = (quadraticB * quadraticB) - (4.0 * quadraticA * quadraticC);

			if (discriminant < 0.0 || quadraticA <= 0.0)
				continue;

			float rootValue = Math.Sqrt(discriminant);
			float firstDistance = (-quadraticB - rootValue) / (2.0 * quadraticA);
			float secondDistance = (-quadraticB + rootValue) / (2.0 * quadraticA);
			float startX = centerX + offsetX + (directionX * firstDistance);
			float startY = centerY + offsetY + (directionY * firstDistance);
			float endX = centerX + offsetX + (directionX * secondDistance);
			float endY = centerY + offsetY + (directionY * secondDistance);
			DrawCanvasLine(startX, startY, endX, endY, lineWidthPixels, color);
		}
	}

	protected void DrawEllipseOutline(float centerX, float centerY, float radiusX, float radiusY, int color, float lineWidth, int segments)
	{
		int outlineAlpha = (color >> 24) & 255;
		if (outlineAlpha <= 0 || segments < 3 || lineWidth <= 0.0 || radiusX <= 0.0 || radiusY <= 0.0)
			return;

		float angleStepDegrees = 360.0 / segments;
		float previousX = centerX + radiusX;
		float previousY = centerY;

		for (int outlineIndex = 1; outlineIndex <= segments; outlineIndex++)
		{
			float angleRadians = (outlineIndex * angleStepDegrees) * Math.DEG2RAD;
			float nextX = centerX + (Math.Cos(angleRadians) * radiusX);
			float nextY = centerY + (Math.Sin(angleRadians) * radiusY);
			DrawCanvasLine(previousX, previousY, nextX, nextY, lineWidth, color);
			previousX = nextX;
			previousY = nextY;
		}
	}

	protected bool ResolveMarkerRenderData(ZenCircleMapMarkerData markerData, bool respectExpansionMarkerVisibility, out vector worldPosition, out int outlineColor, out int fillColor)
	{
		if (!markerData)
			return false;

		worldPosition = markerData.Position;
		outlineColor = markerData.OutlineColor;
		fillColor = markerData.FillColor;

		if (markerData.ExpansionMarkerLink == ZenCircleMapMarkerConstants.NO_BUILD_ZONE_LINK)
		{
			#ifdef DZ_Expansion_Navigation
			return ZenCircleMapMarkerNoBuildVisibility.IsVisible(respectExpansionMarkerVisibility);
			#else
			return true;
			#endif
		}

		if (!markerData.HasExpansionLink())
			return true;

		#ifdef DZ_Expansion_Navigation
		return ResolveExpansionMarkerRenderData(markerData.ExpansionMarkerLink, respectExpansionMarkerVisibility, worldPosition, outlineColor, fillColor);
		#else
		return false;
		#endif
	}

	#ifdef DZ_Expansion_Navigation
	protected bool ResolveExpansionMarkerRenderData(string expansionMarkerLink, bool respectExpansionMarkerVisibility, out vector worldPosition, inout int outlineColor, inout int fillColor)
	{
		ExpansionMarkerModule expansionMarkerModule;
		if (!CF_Modules<ExpansionMarkerModule>.Get(expansionMarkerModule))
			return false;

		ExpansionMarkerClientData expansionClientData = expansionMarkerModule.GetData();
		if (!expansionClientData)
			return false;

		ExpansionMarkerData expansionMarkerData;
		bool inheritMarkerColor = false;

		if (expansionMarkerLink.IndexOf("uid:") == 0)
		{
			string expansionMarkerUID = expansionMarkerLink.Substring(4, expansionMarkerLink.Length() - 4);
			expansionMarkerData = expansionClientData.ServerGet(expansionMarkerUID);
		}
		else if (expansionMarkerLink.IndexOf("name:") == 0)
		{
			string expansionMarkerName = expansionMarkerLink.Substring(5, expansionMarkerLink.Length() - 5);

			for (int expansionMarkerIndex = 0; expansionMarkerIndex < expansionClientData.ServerCount(); expansionMarkerIndex++)
			{
				ExpansionMarkerData expansionMarkerCandidate = expansionClientData.ServerGet(expansionMarkerIndex);
				if (expansionMarkerCandidate && expansionMarkerCandidate.GetName() == expansionMarkerName)
				{
					expansionMarkerData = expansionMarkerCandidate;
					break;
				}
			}
		}
		else if (expansionMarkerLink.IndexOf("personaluidcolor:") == 0)
		{
			string personalColorMarkerUID = expansionMarkerLink.Substring(17, expansionMarkerLink.Length() - 17);
			expansionMarkerData = expansionClientData.PersonalGetByUID(personalColorMarkerUID);
			inheritMarkerColor = true;
		}
		else if (expansionMarkerLink.IndexOf("personaluid:") == 0)
		{
			string personalMarkerUID = expansionMarkerLink.Substring(12, expansionMarkerLink.Length() - 12);
			expansionMarkerData = expansionClientData.PersonalGetByUID(personalMarkerUID);
		}

		if (!expansionMarkerData)
			return false;

		if (respectExpansionMarkerVisibility)
		{
			if (!expansionMarkerData.IsMapVisible())
				return false;

			if (!expansionMarkerModule.IsMapVisible(expansionMarkerData.GetType()))
				return false;
		}

		worldPosition = expansionMarkerData.GetPosition();

		if (inheritMarkerColor)
		{
			int linkedMarkerColor = expansionMarkerData.GetColor();
			int linkedRed = (linkedMarkerColor >> 16) & 255;
			int linkedGreen = (linkedMarkerColor >> 8) & 255;
			int linkedBlue = linkedMarkerColor & 255;
			int existingOutlineAlpha = (outlineColor >> 24) & 255;
			int existingFillAlpha = (fillColor >> 24) & 255;
			outlineColor = ARGB(existingOutlineAlpha, linkedRed, linkedGreen, linkedBlue);
			fillColor = ARGB(existingFillAlpha, linkedRed, linkedGreen, linkedBlue);
		}

		return true;
	}
	#endif
};
