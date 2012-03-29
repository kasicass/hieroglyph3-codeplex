//--------------------------------------------------------------------------------
// This file is a portion of the Hieroglyph 3 Rendering Engine.  It is distributed
// under the MIT License, available in the root of this distribution and 
// at the following URL:
//
// http://www.opensource.org/licenses/mit-license.php
//
// Copyright (c) 2003-2010 Jason Zink 
//--------------------------------------------------------------------------------
#include "App.h"
#include "Log.h"

#include <sstream>

#include "EventManager.h"
#include "EvtFrameStart.h"
#include "EvtChar.h"
#include "EvtKeyUp.h"
#include "EvtKeyDown.h"

#include "ScriptManager.h"

#include "GeometryLoaderDX11.h"
#include "GeometryGeneratorDX11.h"
#include "MaterialGeneratorDX11.h"
#include "RasterizerStateConfigDX11.h"

#include "IParameterManager.h"

#include "ShaderResourceParameterWriterDX11.h"
#include "RotationController.h"

#include "ActorGenerator.h"

using namespace Glyph3;
//--------------------------------------------------------------------------------
App AppInstance; // Provides an instance of the application
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
App::App()
{
}
//--------------------------------------------------------------------------------
bool App::ConfigureEngineComponents()
{
	if ( !ConfigureRenderingEngineComponents( 1024, 800, D3D_FEATURE_LEVEL_11_0 ) ) {
		return( false );
	}

	if ( !ConfigureRenderingSetup() ) {
		return( false );
	}

	return( true );
}
//--------------------------------------------------------------------------------
void App::ShutdownEngineComponents()
{
	ShutdownRenderingSetup();
	ShutdownRenderingEngineComponents();
}
//--------------------------------------------------------------------------------
void App::Initialize()
{

	// Create the light parameters for use with this effect

	Vector4f LightParams = Vector4f( 0.2f, 0.2f, 0.2f, 0.7f );
	m_pLightColor = m_pRenderer11->m_pParamMgr->GetVectorParameterRef( std::wstring( L"LightColor" ) );
	m_pLightColor->InitializeParameterData( &LightParams );

	Vector4f LightPosition = Vector4f( -1000.0f, 200.0f, 0.0f, 0.0f );
	m_pLightPosition = m_pRenderer11->m_pParamMgr->GetVectorParameterRef( std::wstring( L"LightPositionWS" ) );
	m_pLightPosition->InitializeParameterData( &LightPosition );


	// Configure the camera, and the render view that will produce an image of the 
	// from the camera's point of view of the scene.

	m_pCamera->GetNode()->Position() = Vector3f( 7.0f, 7.0f, -10.0f );
	m_pCamera->GetNode()->Rotation().Rotation( Vector3f( 0.3f, -0.4f, 0.0f ) );
	m_pRenderView->SetBackColor( Vector4f( 0.1f, 0.1f, 0.3f, 0.0f ) );


	// Create a Kinect render view, and then attach it to the color and depth
	// visualization actors.  This will ensure that the render view is executed
	// prior to being used by either actor.

	m_pKinectView = new ViewKinect( *m_pRenderer11 );

	m_pColorActor = ActorGenerator::GenerateVisualizationTexture2D( 
		*m_pRenderer11, m_pKinectView->GetColorResource(), 0 );
	m_pDepthActor = ActorGenerator::GenerateVisualizationTexture2D( 
		*m_pRenderer11, m_pKinectView->GetDepthResource(),
		MaterialGeneratorDX11::GenerateKinectDepthBufferMaterial(*m_pRenderer11) );
	
	m_pColorActor->GetBody()->GetMaterial()->Params[VT_PERSPECTIVE].vViews.add( m_pKinectView );
	m_pDepthActor->GetBody()->GetMaterial()->Params[VT_PERSPECTIVE].vViews.add( m_pKinectView );
	
	m_pColorActor->GetNode()->Position() = Vector3f( -5.5f, 5.5f, 5.0f );
	m_pColorActor->GetNode()->Rotation().RotationY( -0.7f );
	m_pDepthActor->GetNode()->Position() = Vector3f( -5.5f, 0.0f, 5.0f );
	m_pDepthActor->GetNode()->Rotation().RotationY( -0.7f );



	// Create an actor to represent the 3D reconstruction of the scene.

	Actor* m_pDepthMapViewer = new Actor();

	GeometryPtr pGeometry = GeometryPtr( new GeometryDX11() );
	GeometryGeneratorDX11::GenerateTexturedPlane( pGeometry, 320, 240 );
	pGeometry->LoadToBuffers();
	pGeometry->SetPrimitiveType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	MaterialPtr pMaterial = MaterialGeneratorDX11::GenerateKinectReconstructionMaterial( *m_pRenderer11 );

	m_pDepthMapViewer->GetBody()->SetGeometry( pGeometry );
	m_pDepthMapViewer->GetBody()->SetMaterial( pMaterial );
	m_pDepthMapViewer->GetBody()->GetMaterial()->Params[VT_PERSPECTIVE].vViews.add( m_pKinectView );

	m_pDepthMapViewer->GetNode()->Position() = Vector3f( 1.0f, 0.0f, 2.0f );
	m_pDepthMapViewer->GetNode()->Rotation().RotationX( 0.0f );
	m_pDepthMapViewer->GetNode()->Scale() = 7.0f;



	// Create a skeleton actor to show the joint tracking in the 3D representation.
	// It is then bound to the kinect render view for updating its values from the
	// SDK calculated joints.  It is then connected to the depth map viewer so that
	// any scaling applied matched between both of them.

	m_pSkeletonActor = new KinectSkeletonActor();
	m_pKinectView->SetSkeletonActor( m_pSkeletonActor );
	m_pDepthMapViewer->GetNode()->AttachChild( m_pSkeletonActor->GetNode() );


	// Add all our objects to the scene for rendering.

	m_pScene->AddEntity( m_pColorActor->GetNode() );
	m_pScene->AddEntity( m_pDepthActor->GetNode() );
	m_pScene->AddEntity( m_pDepthMapViewer->GetNode() );

	

	m_pRenderer11->SetMultiThreadingState( false );
}
//--------------------------------------------------------------------------------
void App::Update()
{
	// Update the timer to determine the elapsed time since last frame.  This can 
	// then used for animation during the frame.

	m_pTimer->Update();


	// Send an event to everyone that a new frame has started.  This will be used
	// in later examples for using the material system with render views.

	EventManager::Get()->ProcessEvent( new EvtFrameStart( *m_pTimer ) );


	std::wstringstream out;
	out << L"Hieroglyph 3 : Kinect Playground" << std::endl;
	out << L"Color and depth are represented on the left, with a 3D reconstruction on the right." << std::endl;
	out << L"FPS: " << m_pTimer->Framerate();

	m_pTextOverlayView->WriteText( out.str(), Matrix4f::Identity(), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) );

	// Update the scene, and then render all cameras within the scene.

	m_pScene->Update( m_pTimer->Elapsed() );
	m_pScene->Render( m_pRenderer11 );


	// Perform the rendering and presentation for the window.

	m_pRenderer11->Present( m_pWindow->GetHandle(), m_pWindow->GetSwapChain() );


	// Save a screenshot if desired.  This is done by pressing the 's' key, which
	// demonstrates how an event is sent and handled by an event listener (which
	// in this case is the application object itself).

	if ( m_bSaveScreenshot  )
	{
		m_bSaveScreenshot = false;
		m_pRenderer11->pImmPipeline->SaveTextureScreenShot( 0, GetName(), D3DX11_IFF_BMP );
	}
}
//--------------------------------------------------------------------------------
void App::Shutdown()
{
	SAFE_DELETE( m_pColorActor );
	SAFE_DELETE( m_pDepthActor );
	SAFE_DELETE( m_pSkeletonActor );
	SAFE_DELETE( m_pKinectView );

	// Print the framerate out for the log before shutting down.

	std::wstringstream out;
	out << L"Max FPS: " << m_pTimer->MaxFramerate();
	Log::Get().Write( out.str() );
}
//--------------------------------------------------------------------------------
bool App::HandleEvent( IEvent* pEvent )
{
	eEVENT e = pEvent->GetEventType();

	if ( e == SYSTEM_KEYBOARD_KEYDOWN )
	{
		EvtKeyDown* pKeyDown = (EvtKeyDown*)pEvent;

		unsigned int key = pKeyDown->GetCharacterCode();

		//return( true );
	}
	else if ( e == SYSTEM_KEYBOARD_KEYUP )
	{
		EvtKeyUp* pKeyUp = (EvtKeyUp*)pEvent;

		unsigned int key = pKeyUp->GetCharacterCode();

		if ( key == VK_ESCAPE ) // 'Esc' Key - Exit the application
		{
			this->RequestTermination();
			return( true );
		}
		else if ( key == VK_SPACE ) // 'Space' Key - Save a screen shot for the next frame
		{
			m_bSaveScreenshot = true;
			return( true );
		}
	}

	// Call the parent class's event handler if we haven't handled the event.

	return( RenderApplication::HandleEvent( pEvent ) );
}
//--------------------------------------------------------------------------------
std::wstring App::GetName( )
{
	return( std::wstring( L"KinectPlayground" ) );
}
//--------------------------------------------------------------------------------