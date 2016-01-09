#ifndef CWORLDEDITOR_H
#define CWORLDEDITOR_H

#include "Editor/INodeEditor.h"
#include "Editor/CGizmo.h"

#include <Common/CTimer.h>
#include <Common/EKeyInputs.h>
#include <Math/CRay.h>
#include <Math/ETransformSpace.h>
#include <Core/Render/CRenderer.h>
#include <Core/Resource/CGameArea.h>
#include <Core/Resource/CWorld.h>
#include <Core/Resource/TResPtr.h>
#include <Core/Scene/CScene.h>
#include <Core/SRayIntersection.h>

#include <QComboBox>
#include <QList>
#include <QMainWindow>
#include <QTimer>
#include <QUndoStack>

namespace Ui {
class CWorldEditor;
}

class CWorldEditor : public INodeEditor
{
    Q_OBJECT
    Ui::CWorldEditor *ui;

    TResPtr<CWorld> mpWorld;
    TResPtr<CGameArea> mpArea;
    QTimer mRefreshTimer;

public:
    explicit CWorldEditor(QWidget *parent = 0);
    ~CWorldEditor();
    bool eventFilter(QObject *pObj, QEvent *pEvent);
    void SetArea(CWorld *pWorld, CGameArea *pArea);
    CGameArea* ActiveArea();

    // Update UI
    void UpdateStatusBar();

public slots:
    void UpdateGizmoUI();
    void UpdateSelectionUI();

protected:
    void GizmoModeChanged(CGizmo::EGizmoMode mode);
    void UpdateCursor();

private slots:
    void RefreshViewport();
    void UpdateCameraOrbit();
    void OnCameraSpeedChange(double speed);
    void OnTransformSpinBoxModified(CVector3f value);
    void OnTransformSpinBoxEdited(CVector3f value);
    void on_ActionDrawWorld_triggered();
    void on_ActionDrawCollision_triggered();
    void on_ActionDrawObjects_triggered();
    void on_ActionDrawLights_triggered();
    void on_ActionDrawSky_triggered();
    void on_ActionNoLighting_triggered();
    void on_ActionBasicLighting_triggered();
    void on_ActionWorldLighting_triggered();
    void on_ActionNoBloom_triggered();
    void on_ActionBloomMaps_triggered();
    void on_ActionFakeBloom_triggered();
    void on_ActionBloom_triggered();
    void on_ActionDisableBackfaceCull_triggered();
    void on_ActionDisableAlpha_triggered();
    void on_ActionEditLayers_triggered();
    void on_ActionIncrementGizmo_triggered();
    void on_ActionDecrementGizmo_triggered();
    void on_ActionDrawObjectCollision_triggered();
    void on_ActionGameMode_triggered();
    void on_ActionSelectAll_triggered();
    void on_ActionInvertSelection_triggered();
};

#endif // CWORLDEDITOR_H
