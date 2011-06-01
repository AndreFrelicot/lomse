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

#include "lomse_chord_engraver.h"

#include "lomse_im_note.h"
#include "lomse_shape_note.h"
#include "lomse_gm_basic.h"
#include "lomse_note_engraver.h"
#include "lomse_score_meter.h"
#include "lomse_engraving_options.h"

#include <cstdlib>      //abs


namespace lomse
{


//=======================================================================================
// ClefEngraver implementation
//=======================================================================================
ChordEngraver::ChordEngraver(LibraryScope& libraryScope, ScoreMeter* pScoreMeter)
    : RelAuxObjEngraver(libraryScope, pScoreMeter)
    , m_pBaseNoteData(NULL)
{
}

//---------------------------------------------------------------------------------------
ChordEngraver::~ChordEngraver()
{
    std::list<ChordNoteData*>::iterator it;
    for (it = m_notes.begin(); it != m_notes.end(); ++it)
        delete *it;
    m_notes.clear();
}

//---------------------------------------------------------------------------------------
void ChordEngraver::set_start_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                       GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                       int iSystem, int iCol, UPoint pos)
{
    m_iInstr = iInstr;
    m_iStaff = iStaff;
    m_pChord = dynamic_cast<ImoChord*>(pAO);

    add_note(pSO, pStaffObjShape);
}

//---------------------------------------------------------------------------------------
void ChordEngraver::set_middle_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                         GmoShape* pStaffObjShape, int iInstr,
                                         int iStaff, int iSystem, int iCol)
{
    add_note(pSO, pStaffObjShape);
}

//---------------------------------------------------------------------------------------
void ChordEngraver::set_end_staffobj(ImoAuxObj* pAO, ImoStaffObj* pSO,
                                      GmoShape* pStaffObjShape, int iInstr, int iStaff,
                                      int iSystem, int iCol)
{
    add_note(pSO, pStaffObjShape);
}

//---------------------------------------------------------------------------------------
int ChordEngraver::create_shapes()
{
    decide_on_stem_direction();
    layout_noteheads();
    layout_accidentals();
    add_stem_and_flag();
    set_anchor_offset();
    return 0;
}

//---------------------------------------------------------------------------------------
void ChordEngraver::add_note(ImoStaffObj* pSO, GmoShape* pStaffObjShape)
{
    ImoNote* pNote = dynamic_cast<ImoNote*>(pSO);
    GmoShapeNote* pNoteShape = dynamic_cast<GmoShapeNote*>(pStaffObjShape);
    int posOnStaff = pNoteShape->get_pos_on_staff();

    if (m_notes.size() == 0)
    {
        ChordNoteData* pData = new ChordNoteData(pNote, pNoteShape, posOnStaff, m_iInstr);
	    m_notes.push_back(pData);
        m_pBaseNoteData = pData;
        m_stemWidth = tenths_to_logical(LOMSE_STEM_THICKNESS);
    }
    else
    {
        //keep notes sorted by pitch
        DiatonicPitch newPitch(pNote->get_step(), pNote->get_octave());
        std::list<ChordNoteData*>::iterator it;
        for (it = m_notes.begin(); it != m_notes.end(); ++it)
        {
            DiatonicPitch curPitch((*it)->pNote->get_step(), (*it)->pNote->get_octave());
            if (newPitch < curPitch)
            {
                ChordNoteData* pData =
                    new ChordNoteData(pNote, pNoteShape, posOnStaff, m_iInstr);
	            m_notes.insert(it, 1, pData);
                return;
            }
        }
        ChordNoteData* pData = new ChordNoteData(pNote, pNoteShape, posOnStaff, m_iInstr);
	    m_notes.push_back(pData);
    }
}

//---------------------------------------------------------------------------------------
void ChordEngraver::decide_on_stem_direction()
{
    //  Rules (taken from www.coloradocollege.edu/dept/mu/mu2/musicpress/NotesStems.html
    //
    //  a) Two notes in chord:
    //    a1. If the interval above the middle line is greater than the interval below
    //      the middle line: downward stems. i.e. (a4,d5) (f4,f5) (a4,g5)
    //      ==>   (MaxNotePos + MinNotePos)/2 > MiddleLinePos
    //
    //    a2. If the interval below the middle line is greater than the interval above
    //      the middle line: upward stems. i.e. (e4,c5)(g4,c5)(d4,e5)
    //
    //    a3. If the two notes are at the same distance from the middle line: stem can
    //      go in either direction, but most engravers prefer downward stems.
    //      i.e. (g4.d5)(a4,c5)
    //
    //
    //  b) More than two notes in chord:
    //
    //    b1. If the interval of the highest note above the middle line is greater than
    //      the interval of the lowest note below the middle line: downward stems.
    //      ==>   same than a1
    //
    //    b2. If the interval of the lowest note below the middle line is greater than
    //      the interval of the highest note above the middle line: upward stems.
    //      ==>   same than a2
    //
    //    b3. If the highest and the lowest notes are the same distance from the middle
    //      line use the majority rule to determine stem direction: If the majority of
    //      the notes are above the middle: downward stems. Else: upward stems.
    //      ==>   Mean(NotePos) > MiddleLinePos -> downward
    //
    //  Additional rules (mine):
    //  c) chords without stem (notes longer than half notes):
    //    c1. layout as if stem was up

    ImoNote* pBaseNote = get_base_note();
    m_noteType = pBaseNote->get_note_type();
    int stemType = pBaseNote->get_stem_direction();

    m_fHasStem = m_noteType >= k_half
                 && stemType != k_stem_none;
    m_fHasFlag = m_fHasStem && m_noteType > k_quarter
                 && !is_chord_beamed();


    if (m_noteType < k_half)
        m_fStemDown = false;                    //c1. layout as if stem up

    else if (stemType == k_stem_up)
        m_fStemDown = false;                    //force stem up

    else if (stemType == k_stem_down)
        m_fStemDown = true;                     //force stem down

    else if (stemType == k_stem_none)
        m_fStemDown = false;                    //c1. layout as if stem up

    else if (stemType == k_stem_default)     //as decided by program
    {
        //majority rule
        int weight = 0;
        std::list<ChordNoteData*>::iterator it;
        for(it=m_notes.begin(); it != m_notes.end(); ++it)
            weight += (*it)->posOnStaff;

        m_fStemDown = ( weight >= 6 * int(m_notes.size()) );
    }
}

//---------------------------------------------------------------------------------------
bool ChordEngraver::is_chord_beamed()
{
    return get_base_note()->is_beamed();
}

//---------------------------------------------------------------------------------------
void ChordEngraver::layout_noteheads()
{
    align_noteheads();
    arrange_notheads_to_avoid_collisions();
}

//---------------------------------------------------------------------------------------
void ChordEngraver::align_noteheads()
{
    LUnits maxLeft = 0.0f;
    std::list<ChordNoteData*>::iterator it;
    for (it = m_notes.begin(); it != m_notes.end(); ++it)
        maxLeft = max(maxLeft, (*it)->pNoteShape->get_notehead_left());

    for (it = m_notes.begin(); it != m_notes.end(); ++it)
    {
        LUnits xShift = maxLeft - (*it)->pNoteShape->get_notehead_left();
        if (xShift > 0.0f)
        {
            USize shift(xShift, 0.0f);
            (*it)->pNoteShape->shift_origin(shift);
        }
    }

}

//---------------------------------------------------------------------------------------
void ChordEngraver::arrange_notheads_to_avoid_collisions()
{
    //Arrange noteheads at left/right of stem to avoid collisions
    //The algorithm asumes that the stem direction has been computed and that
    //notes are already sorted by pitch. The algorithm is simple:
    // 1. If the stem goes down, start with the highest note and go downwards. Else
    //    start with the lowes note and go upwards.
    // 2. Place each note on the normal side of the stem, unless the interval with the
    //    previous note is a second and previous not is not reversed.

    int nPosPrev = 1000;    // a very high number not posible in real world
    m_fSomeNoteReversed = false;
    if (m_fStemDown)
    {
        std::list<ChordNoteData*>::reverse_iterator it;
        for (it = m_notes.rbegin(); it != m_notes.rend(); ++it)
        {
		    //do processing
		    int pos = (*it)->posOnStaff;
		    if (abs(nPosPrev - pos) < 2)
		    {
			    //collision. Reverse position of this notehead
			    (*it)->fNoteheadReversed = true;
			    m_fSomeNoteReversed = true;
			    reverse_notehead((*it)->pNoteShape);
			    nPosPrev = 1000;
		    }
		    else
			    nPosPrev = pos;
        }
    }
    else
    {
        std::list<ChordNoteData*>::iterator it;
        for (it = m_notes.begin(); it != m_notes.end(); ++it)
        {
		    //do processing
		    int pos = (*it)->posOnStaff;
		    if (abs(nPosPrev - pos) < 2)
		    {
			    //collision. Reverse position of this notehead
			    (*it)->fNoteheadReversed = true;
			    m_fSomeNoteReversed = true;
			    reverse_notehead((*it)->pNoteShape);
			    nPosPrev = 1000;
		    }
		    else
			    nPosPrev = pos;
        }
    }
}

//---------------------------------------------------------------------------------------
void ChordEngraver::reverse_notehead(GmoShapeNote* pNoteShape)
{
    LUnits xShift;
    if (is_stem_up())
        xShift = pNoteShape->get_notehead_width() - m_stemWidth;
    else
        xShift =  -pNoteShape->get_notehead_width() + m_stemWidth;

    USize shift(xShift, 0.0f);
    pNoteShape->shift_origin(shift);
}

//---------------------------------------------------------------------------------------
void ChordEngraver::layout_accidentals()
{
    std::list<ChordNoteData*>::reverse_iterator it;
    for(it = m_notes.rbegin(); it != m_notes.rend(); ++it)
    {
        GmoShapeNote* pNoteShape = (*it)->pNoteShape;
        GmoShapeAccidentals* pCurAcc = pNoteShape->get_accidentals_shape();
        pNoteShape->unlock();

        if (pCurAcc)
        {
            //check if conflict with next two noteheads
            std::list<ChordNoteData*>::reverse_iterator itCur = it;
            ++itCur;
            if (itCur != m_notes.rend())
            {
                GmoShapeNotehead* pHead = (*itCur)->pNoteShape->get_notehead_shape();
                shift_acc_if_confict_with_shape(pCurAcc, pHead);
            }
            ++itCur;
            if (itCur != m_notes.rend())
            {
                GmoShapeNotehead* pHead = (*itCur)->pNoteShape->get_notehead_shape();
                shift_acc_if_confict_with_shape(pCurAcc, pHead);
            }

            //check if conflict with two previous noteheads
            if (it != m_notes.rbegin())
            {
                itCur = it;
                --itCur;
                GmoShapeNotehead* pHead = (*itCur)->pNoteShape->get_notehead_shape();
                shift_acc_if_confict_with_shape(pCurAcc, pHead);
                if (itCur != m_notes.rbegin())
                {
                    --itCur;
                    GmoShapeNotehead* pHead = (*itCur)->pNoteShape->get_notehead_shape();
                    shift_acc_if_confict_with_shape(pCurAcc, pHead);
                }
            }

            //check if conflict with any previous accidental
            shift_accidental_if_conflict_with_previous(pCurAcc, it);
        }
    }
}

//---------------------------------------------------------------------------------------
void ChordEngraver::shift_accidental_if_conflict_with_previous(
                                GmoShapeAccidentals* pCurAcc,
                                std::list<ChordNoteData*>::reverse_iterator& itCur)
{
	std::list<ChordNoteData*>::reverse_iterator it;
	for(it = m_notes.rbegin(); it != itCur; ++it)
	{
		GmoShapeNote* pNoteShape = (*it)->pNoteShape;
		GmoShapeAccidentals* pPrevAcc = pNoteShape->get_accidentals_shape();
        if (pPrevAcc)
            shift_acc_if_confict_with_shape(pCurAcc, pPrevAcc);
	}
}

//---------------------------------------------------------------------------------------
void ChordEngraver::shift_acc_if_confict_with_shape(GmoShapeAccidentals* pCurAcc,
                                                    GmoShape* pShape)
{
    LUnits xOverlap = check_if_overlap(pShape, pCurAcc);
    if (xOverlap > 0.0f)
    {
        LUnits space = tenths_to_logical(LOMSE_SPACE_BETWEEN_ACCIDENTALS);
        LUnits shift = pCurAcc->get_right() - pShape->get_left();
        xOverlap = - (shift + space);
        if (xOverlap != 0.0f)
        {
            USize shift(xOverlap, 0.0f);
            pCurAcc->shift_origin(shift);
        }
    }
}

//---------------------------------------------------------------------------------------
void ChordEngraver::set_anchor_offset()
{
	std::list<ChordNoteData*>::iterator it;
	for(it = m_notes.begin(); it != m_notes.end(); ++it)
	{
		GmoShapeNote* pNoteShape = (*it)->pNoteShape;
		pNoteShape->lock();

        //compute anchor pos
        LUnits offset;
        if (!m_fSomeNoteReversed)
            //a) if no reversed noteads, anchor is notehead x left. Stem direction
            //   doesn't matter.
            offset = pNoteShape->get_notehead_left();

        else
        {
            //b) At least a note with reversed notehead:
            if (is_stem_down())
            {
                //b.1) if stem goes down:
                if ((*it)->fNoteheadReversed)
                    //b.1.1) if note is reversed, anchor is x left of notehead.
                    offset = pNoteShape->get_notehead_left();
                else
                    //b.1.2) if note is not reversed, anchor is x left of notehead plus
                    //       stem width minus notehead width.
                    offset = pNoteShape->get_notehead_left() + m_stemWidth
                             - pNoteShape->get_notehead_width();
            }
            else
            {
                //b.2) if stem goes up:
                if (!(*it)->fNoteheadReversed)
                    //b.2.1) if note is not reversed, anchor is x left of notehead.
                    offset = pNoteShape->get_notehead_left();
                else
                    //b.2.2) if note is reversed, anchor is x left of notehead plus
                    //       stem width minus notehead width.
                    offset = pNoteShape->get_notehead_left() + m_stemWidth
                             - pNoteShape->get_notehead_width();
            }
        }
        offset = pNoteShape->get_origin().x - offset;

        if (offset != 0.0f)
            pNoteShape->set_anchor_offset(offset);
	}
}

//---------------------------------------------------------------------------------------
LUnits ChordEngraver::check_if_overlap(GmoShape* pShape, GmoShape* pNewShape)
{
    URect overlap = pShape->get_bounds();
    overlap.intersection( pNewShape->get_bounds() );
    return overlap.get_width();
}

//---------------------------------------------------------------------------------------
LUnits ChordEngraver::check_if_accidentals_overlap(GmoShapeAccidentals* pPrevAcc,
                                                   GmoShapeAccidentals* pCurAcc)
{
    URect overlap = pPrevAcc->get_bounds();
    overlap.intersection( pCurAcc->get_bounds() );
    return overlap.get_width();
}

//---------------------------------------------------------------------------------------
void ChordEngraver::add_stem_and_flag()
{

    if (!has_stem())
        return;

    //the stem length must be increased with the distance from min note to max note.
    GmoShapeNote* pMinNoteShape = m_notes.front()->pNoteShape;
    GmoShapeNote* pMaxNoteShape = m_notes.back()->pNoteShape;
    GmoShapeNotehead* pMinHeadShape = pMinNoteShape->get_notehead_shape();
    GmoShapeNotehead* pMaxHeadShape = pMaxNoteShape->get_notehead_shape();
    LUnits uExtraLenght = pMinHeadShape->get_top() - pMaxHeadShape->get_top();

    //stem and the flag will computed for max/min note, depending on stem direction
    ChordNoteData* pData = (is_stem_down() ? m_notes.back() : m_notes.front());
    ImoNote* pNote = pData->pNote;
    GmoShapeNote* pNoteShape = pData->pNoteShape;
    int instr = pData->iInstr;
    int staff = pNote->get_staff();
    int nPosOnStaff = pData->posOnStaff;

    //but the stem is added to base note shape
    GmoShapeNote* pBaseNoteShape = m_pBaseNoteData->pNoteShape;
    Tenths length = NoteEngraver::get_standard_stem_length(nPosOnStaff, is_stem_down());
    LUnits stemLength = tenths_to_logical(length) + uExtraLenght;
    StemFlagEngraver engrv(m_libraryScope, m_pMeter, pNote, instr, staff);

    engrv.add_stem_flag(pNoteShape, pBaseNoteShape, m_noteType, is_stem_down(),
                        has_flag(), stemLength, Color(0,0,0));
}

//---------------------------------------------------------------------------------------
LUnits ChordEngraver::tenths_to_logical(Tenths value)
{
    return m_pMeter->tenths_to_logical(value, m_iInstr, m_iStaff);
}


}  //namespace lomse