//---------------------------------------------------------------------------------------
//  This file is part of the Lomse library.
//  Copyright (c) 2010-2011 Lomse project
//
//  Lomse is free software; you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software Foundation,
//  either version 3 of the License, or (at your option) any later version.
//
//  Lomse is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with Lomse; if not, see <http://www.gnu.org/licenses/>.
//
//  For any comment, suggestion or feature request, please contact the manager of
//  the project at cecilios@users.sourceforge.net
//
//---------------------------------------------------------------------------------------

#include "lomse_tie_engraver.h"

#include "lomse_internal_model.h"
#include "lomse_im_note.h"
#include "lomse_engraving_options.h"
#include "lomse_shape_tie.h"
#include "lomse_shape_note.h"
#include "lomse_score_meter.h"


namespace lomse
{

//some macros to improve code reading
#define START       ImoBezierInfo::k_start
#define END         ImoBezierInfo::k_end
#define CTROL1      ImoBezierInfo::k_ctrol1
#define CTROL2      ImoBezierInfo::k_ctrol2

//---------------------------------------------------------------------------------------
// TieEngraver implementation
//---------------------------------------------------------------------------------------
TieEngraver::TieEngraver(LibraryScope& libraryScope, ScoreMeter* pScoreMeter,
                         LUnits uStaffLeft, LUnits uStaffRight)
    : RelAuxObjEngraver(libraryScope, pScoreMeter)
    , m_uStaffLeft(uStaffLeft)
    , m_uStaffRight(uStaffRight)
    , m_numShapes(0)
{
}

//---------------------------------------------------------------------------------------
void TieEngraver::set_start_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                     GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                     int iSystem, int iCol, UPoint pos)
{
    m_pTie = dynamic_cast<ImoTie*>( pAO );

    m_pStartNote = dynamic_cast<ImoNote*>(pSO);
    m_pStartNoteShape = dynamic_cast<GmoShapeNote*>(pStaffObjShape);
    m_iInstr = iInstr;
    m_iStaff = iStaff;
    m_pos = pos;

    m_shapesInfo[0].iCol = iCol;
    m_shapesInfo[0].iInstr = iInstr;
    m_shapesInfo[0].iSystem = iSystem;
}

//---------------------------------------------------------------------------------------
void TieEngraver::set_end_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                   GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                   int iSystem, int iCol)
{
    m_pEndNote = dynamic_cast<ImoNote*>(pSO);
    m_pEndNoteShape = dynamic_cast<GmoShapeNote*>(pStaffObjShape);
    m_shapesInfo[1].iCol = iCol;
    m_shapesInfo[1].iInstr = iInstr;
    m_shapesInfo[1].iSystem = iSystem;
}

//---------------------------------------------------------------------------------------
int TieEngraver::create_shapes()
{
    decide_placement();
    decide_if_one_or_two_arches();
    if (two_arches_needed())
        create_two_shapes();
    else
        create_one_shape();
    return m_numShapes;
}

//---------------------------------------------------------------------------------------
void TieEngraver::create_one_shape()
{
    m_numShapes = 1;

    compute_start_point();
    compute_end_point(&m_points1[END]);
    compute_default_control_points(&m_points1[0]);
    add_user_displacements(0, &m_points1[0]);
    m_shapesInfo[0].pShape = new GmoShapeTie(m_pTie, 0, &m_points1[0], m_thickness);

    m_shapesInfo[1].pShape = NULL;
}

//---------------------------------------------------------------------------------------
void TieEngraver::create_two_shapes()
{
    m_numShapes = 2;

    //create first shape
    compute_start_point();
    compute_end_of_staff_point();
    compute_default_control_points(&m_points1[0]);
    add_user_displacements(0, &m_points1[0]);
    m_shapesInfo[0].pShape = new GmoShapeTie(m_pTie, 0, &m_points1[0], m_thickness);

    //create second shape
    compute_end_point(&m_points2[END]);
    compute_start_of_staff_point();
    compute_default_control_points(&m_points2[0]);
    add_user_displacements(1, &m_points2[0]);
    m_shapesInfo[1].pShape = new GmoShapeTie(m_pTie, 0, &m_points2[0], m_thickness);
}

//---------------------------------------------------------------------------------------
void TieEngraver::compute_default_control_points(UPoint* points)
{
    LUnits D = (points+END)->x - (points+START)->x;
    LUnits d = D / 5.8f;
    m_thickness = tenths_to_logical(LOMSE_TIE_MAX_THICKNESS);
    LUnits hc = m_thickness * 3.88f;
    (points+CTROL1)->x = (points+START)->x + d;
    (points+CTROL1)->y = (points+START)->y + (m_fTieBelowNote ? hc : -hc);

    (points+CTROL2)->x = (points+END)->x - d;
    (points+CTROL2)->y = (points+END)->y + (m_fTieBelowNote ? hc : -hc);
}

//---------------------------------------------------------------------------------------
void TieEngraver::compute_start_point()
{
	//x pos: center on notehead
	m_points1[START].x = (m_pStartNoteShape->get_notehead_right() +
                          m_pStartNoteShape->get_notehead_left()) / 2.0f;

    //y pos: 5 tenths appart from notehead
    LUnits space = tenths_to_logical(LOMSE_TIE_VERTICAL_SPACE);

    m_points1[START].y = (m_fTieBelowNote ?
                          m_pStartNoteShape->get_notehead_bottom() + space
                          : m_pStartNoteShape->get_notehead_top() - space );
}

//---------------------------------------------------------------------------------------
void TieEngraver::compute_end_point(UPoint* point)
{
	//x pos: center on notehead
	point->x = (m_pEndNoteShape->get_notehead_right() +
                m_pEndNoteShape->get_notehead_left()) / 2.0f;

    //y pos: 5 tenths appart from notehead
    LUnits space = tenths_to_logical(LOMSE_TIE_VERTICAL_SPACE);

    point->y = (m_fTieBelowNote ?
                 m_pEndNoteShape->get_notehead_bottom() + space
               : m_pEndNoteShape->get_notehead_top() - space );
}

//---------------------------------------------------------------------------------------
void TieEngraver::compute_start_of_staff_point()
{
    m_points2[START].x = m_uStaffLeft;
    m_points2[START].y = m_points2[END].y;
}

//---------------------------------------------------------------------------------------
void TieEngraver::compute_end_of_staff_point()
{
    m_points1[END].x = m_uStaffRight;
    m_points1[END].y = m_points1[START].y;
}

//---------------------------------------------------------------------------------------
void TieEngraver::decide_placement()
{
    m_fTieBelowNote = m_pStartNoteShape->is_up();
}

//---------------------------------------------------------------------------------------
void TieEngraver::decide_if_one_or_two_arches()
{
    m_fTwoArches = (m_shapesInfo[0].iSystem != m_shapesInfo[1].iSystem);
}

//---------------------------------------------------------------------------------------
void TieEngraver::add_user_displacements(int iTie, UPoint* points)
{
    ImoBezierInfo* pBezier = (iTie == 0 ? m_pTie->get_start_bezier()
                                        : m_pTie->get_stop_bezier() );
    if (pBezier)
    {
        for (int i=0; i < 4; i++)
        {
            (points+i)->x += tenths_to_logical(pBezier->get_point(i).x);
            (points+i)->y += tenths_to_logical(pBezier->get_point(i).y);
        }
    }
}

//---------------------------------------------------------------------------------------
LUnits TieEngraver::tenths_to_logical(Tenths tenths)
{
    return m_pMeter->tenths_to_logical(tenths, m_iInstr, m_iStaff);
}

//---------------------------------------------------------------------------------------
int TieEngraver::get_num_shapes()
{
    return m_numShapes;
}

//---------------------------------------------------------------------------------------
ShapeBoxInfo* TieEngraver::get_shape_box_info(int i)
{
    return &m_shapesInfo[i];
}


}  //namespace lomse