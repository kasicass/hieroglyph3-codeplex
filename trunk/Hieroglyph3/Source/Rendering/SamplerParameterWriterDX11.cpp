//--------------------------------------------------------------------------------
// This file is a portion of the Hieroglyph 3 Rendering Engine.  It is distributed
// under the MIT License, available in the root of this distribution and 
// at the following URL:
//
// http://www.opensource.org/licenses/mit-license.php
//
// Copyright (c) 2003-2010 Jason Zink 
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
#include "PCH.h"
#include "SamplerParameterWriterDX11.h"
//--------------------------------------------------------------------------------
using namespace Glyph3;
//--------------------------------------------------------------------------------
SamplerParameterWriterDX11::SamplerParameterWriterDX11()
{
	m_Value = -1;
}
//--------------------------------------------------------------------------------
SamplerParameterWriterDX11::~SamplerParameterWriterDX11()
{
}
//--------------------------------------------------------------------------------
void SamplerParameterWriterDX11::WriteParameter( IParameterManager* pParamMgr )
{
	pParamMgr->SetSamplerParameter( m_pParameter, &m_Value );
}
//--------------------------------------------------------------------------------
void SamplerParameterWriterDX11::SetValue( int Value )
{
	m_Value = Value;
}
//--------------------------------------------------------------------------------