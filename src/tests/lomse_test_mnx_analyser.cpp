//---------------------------------------------------------------------------------------
// This file is part of the Lomse library.
// Lomse is copyrighted work (c) 2010-2018. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright notice, this
//      list of conditions and the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// For any comment, suggestion or feature request, please contact the manager of
// the project at cecilios@users.sourceforge.net
//---------------------------------------------------------------------------------------

#define LOMSE_INTERNAL_API
#include <UnitTest++.h>
#include <iostream>
#include "lomse_build_options.h"

//classes related to these tests
#include "lomse_injectors.h"
#include "lomse_document.h"
#include "lomse_xml_parser.h"
#include "lomse_mnx_analyser.h"
#include "lomse_internal_model.h"
#include "lomse_im_note.h"
#include "lomse_events.h"
#include "lomse_doorway.h"
#include "lomse_im_factory.h"
#include "lomse_time.h"
#include "lomse_import_options.h"

#include <regex>

using namespace UnitTest;
using namespace std;
using namespace lomse;


//---------------------------------------------------------------------------------------
class MnxAnalyserTestFixture
{
public:
    LibraryScope m_libraryScope;
    int m_requestType;
    bool m_fRequestReceived;
    ImoDocument* m_pDoc;


    MnxAnalyserTestFixture()     //SetUp fixture
        : m_libraryScope(cout)
        , m_requestType(k_null_request)
        , m_fRequestReceived(false)
        , m_pDoc(nullptr)
    {
        m_libraryScope.set_default_fonts_path(TESTLIB_FONTS_PATH);
    }

    ~MnxAnalyserTestFixture()    //TearDown fixture
    {
    }

    static void wrapper_lomse_request(void* pThis, Request* pRequest)
    {
        static_cast<MnxAnalyserTestFixture*>(pThis)->on_lomse_request(pRequest);
    }

    void on_lomse_request(Request* pRequest)
    {
        m_fRequestReceived = true;
        m_requestType = pRequest->get_request_type();
        if (m_requestType == k_dynamic_content_request)
        {
            RequestDynamic* pRq = dynamic_cast<RequestDynamic*>(pRequest);
            ImoDynamic* pDyn = dynamic_cast<ImoDynamic*>( pRq->get_object() );
            m_pDoc = pDyn->get_document();
        }
    }

    inline const char* test_name()
    {
        return UnitTest::CurrentTest::Details()->testName;
    }

};


SUITE(MnxAnalyserTest)
{

    //@ mnx element ---------------------------------------------------------------------

    TEST_FIXTURE(MnxAnalyserTestFixture, MnxAnalyser_mnx_001)
    {
        //@00001. empty content: returns empty document
        stringstream errormsg;
        Document doc(m_libraryScope);
        XmlParser parser;
        stringstream expected;
        expected << "Line 0. <mnx>: missing mandatory element <head>." << endl;
        parser.parse_text("<mnx></mnx>");
        MnxAnalyser a(errormsg, m_libraryScope, &doc, &parser);

        XmlNode* tree = parser.get_tree_root();
        ImoObj* pRoot =  a.analyse_tree(tree, "string:");

//        cout << test_name() << endl;
//        cout << "[" << errormsg.str() << "]" << endl;
//        cout << "[" << expected.str() << "]" << endl;
        CHECK( errormsg.str() == expected.str() );
        CHECK( pRoot != nullptr );
        CHECK( pRoot && pRoot->is_document() == true );
        ImoDocument* pDoc = dynamic_cast<ImoDocument*>( pRoot );
        CHECK( pDoc && pDoc->get_num_content_items() == 0 );

        if (pRoot && !pRoot->is_document()) delete pRoot;
    }


    //@ global --------------------------------------------------------------------------

    TEST_FIXTURE(MnxAnalyserTestFixture, MnxAnalyser_global_100)
    {
        //@00100. Error: at least one global element is required.
        //@       Doc with empty score returned

        stringstream errormsg;
        Document doc(m_libraryScope);
        XmlParser parser;
        stringstream expected;
        expected << "Line 0. <cwmnx>: missing mandatory element <global>." << endl;
        parser.parse_text(
            "<mnx>"
            "<head></head>"
            "<score><cwmnx profile='standard'>"
                "<part>"
                "</part>"
            "</cwmnx></score>"
            "</mnx>");
        MnxAnalyser a(errormsg, m_libraryScope, &doc, &parser);

        XmlNode* tree = parser.get_tree_root();
        ImoObj* pRoot =  a.analyse_tree(tree, "string:");

//        cout << test_name() << endl;
//        cout << "[" << errormsg.str() << "]" << endl;
//        cout << "[" << expected.str() << "]" << endl;
        CHECK( errormsg.str() == expected.str() );
        CHECK( pRoot != nullptr );
        CHECK( pRoot && pRoot->is_document() == true );
        ImoDocument* pDoc = dynamic_cast<ImoDocument*>( pRoot );
        CHECK( doc.is_dirty() == true );
        CHECK( pDoc && pDoc->get_num_content_items() == 1 );
//        ImoScore* pScore = dynamic_cast<ImoScore*>( pDoc->get_content_item(0) );
//        pScore->end_of_changes();
//        cout << pScore->to_string_with_ids() << endl;

        if (pRoot && !pRoot->is_document()) delete pRoot;
    }

//    TEST_FIXTURE(MnxAnalyserTestFixture, MnxAnalyser_global_101)
//    {
//
//        //@00101. Error: at least one measure required in global
//
//        stringstream errormsg;
//        Document doc(m_libraryScope);
//        XmlParser parser;
//        stringstream expected;
//        expected << "Line 0. <global>: missing mandatory element <measure>." << endl;
//        parser.parse_text(
//            "<mnx>"
//            "<head></head>"
//            "<score><cwmnx profile='standard'>"
//                "<global></global>"
//                "<part>"
//                "</part>"
//            "</cwmnx></score>"
//            "</mnx>");
//        MnxAnalyser a(errormsg, m_libraryScope, &doc, &parser);
//
//        XmlNode* tree = parser.get_tree_root();
//        ImoObj* pRoot =  a.analyse_tree(tree, "string:");
//
//        cout << test_name() << endl;
//        cout << "[" << errormsg.str() << "]" << endl;
//        cout << "[" << expected.str() << "]" << endl;
//        CHECK( errormsg.str() == expected.str() );
//        CHECK( pRoot != nullptr );
//        CHECK( pRoot && pRoot->is_document() == true );
//        ImoDocument* pDoc = dynamic_cast<ImoDocument*>( pRoot );
//        CHECK( doc.is_dirty() == true );
//        CHECK( pDoc && pDoc->get_num_content_items() == 1 );
//        ImoScore* pScore = dynamic_cast<ImoScore*>( pDoc->get_content_item(0) );
//        pScore->end_of_changes();
//        cout << pScore->to_string_with_ids() << endl;
//
//        if (pRoot && !pRoot->is_document()) delete pRoot;
//    }


    //@ measure -------------------------------------------------------------------------

    TEST_FIXTURE(MnxAnalyserTestFixture, measure_01)
    {
        //@01. MeasuresInfo in Barline but not in Instrument

        stringstream errormsg;
        Document doc(m_libraryScope);
        XmlParser parser;
        stringstream expected;
        expected << "Line 0. <cwmnx>: missing mandatory element <global>." << endl;
        parser.parse_text(
            "<mnx>"
            "<head></head>"
            "<score><cwmnx profile='standard'>"
                "<part>"
                    "<part-name>Piano</part-name>"
                    "<measure>"
                      "<directions>"
                        "<staves number='1'/>"
                        "<clef line='2' sign='G'/>"
                      "</directions>"
                      "<sequence staff='1'>"
                        "<event value='/4'><note pitch='C4'/></event>"
                        "<event value='/2'><note pitch='E4'/></event>"
                      "</sequence>"
                    "</measure>"
                "</part>"
            "</cwmnx></score>"
            "</mnx>");
        MnxAnalyser a(errormsg, m_libraryScope, &doc, &parser);

        XmlNode* tree = parser.get_tree_root();
        ImoObj* pRoot =  a.analyse_tree(tree, "string:");

//        cout << test_name() << endl;
//        cout << "[" << errormsg.str() << "]" << endl;
//        cout << "[" << expected.str() << "]" << endl;
        CHECK( errormsg.str() == expected.str() );
        CHECK( pRoot != nullptr );
        CHECK( pRoot && pRoot->is_document() == true );
        ImoDocument* pDoc = dynamic_cast<ImoDocument*>( pRoot );
        CHECK( pDoc && pDoc->get_num_content_items() == 1 );
        ImoScore* pScore = dynamic_cast<ImoScore*>( pDoc->get_content_item(0) );
        ImoInstrument* pInstr = pScore->get_instrument(0);
        TypeMeasureInfo* pInfo = pInstr->get_last_measure_info();
        CHECK( pInfo == nullptr );
        ImoMusicData* pMD = pInstr->get_musicdata();
        CHECK( pMD != nullptr );

        CHECK( pMD && pMD->get_num_children() == 4 );
        ImoObj::children_iterator it = pMD->begin();    //clef
        CHECK( (*it)->is_clef() );
        ++it;   //note c4
        CHECK( (*it)->is_note() );
        ++it;   //note e4
        CHECK( (*it)->is_note() );
        ++it;   //barline
        CHECK( (*it)->is_barline() );
        ImoBarline* pBarline = dynamic_cast<ImoBarline*>( *it );
        CHECK( pBarline != nullptr );
        CHECK( pBarline && pBarline->get_type() == k_barline_simple );
        CHECK( pBarline && pBarline->is_visible() );
        pInfo = pBarline->get_measure_info();
        CHECK( pInfo != nullptr );
        CHECK( pInfo && pInfo->count == 1 );
//        cout << test_name() << ": count=" << pInfo->count << endl;

        if (pRoot && !pRoot->is_document()) delete pRoot;
    }

    TEST_FIXTURE(MnxAnalyserTestFixture, measure_02)
    {
        //@02. Two measures

        stringstream errormsg;
        Document doc(m_libraryScope);
        XmlParser parser;
        stringstream expected;
        expected << "Line 0. <cwmnx>: missing mandatory element <global>." << endl;
        parser.parse_text(
            "<mnx>"
            "<head></head>"
            "<score><cwmnx profile='standard'>"
                "<part>"
                    "<part-name>Piano</part-name>"
                    "<measure number='1'>"
                      "<directions>"
                        "<staves number='1'/>"
                        "<clef line='2' sign='G'/>"
                      "</directions>"
                      "<sequence staff='1'>"
                        "<event value='/4'><note pitch='C4'/></event>"
                        "<event value='/2'><note pitch='E4'/></event>"
                      "</sequence>"
                    "</measure>"
                    "<measure number='2'>"
                      "<sequence staff='1'>"
                        "<event value='/4'><note pitch='C4'/></event>"
                        "<event value='/2'><note pitch='E4'/></event>"
                      "</sequence>"
                    "</measure>"
                "</part>"
            "</cwmnx></score>"
            "</mnx>");
        MnxAnalyser a(errormsg, m_libraryScope, &doc, &parser);

        XmlNode* tree = parser.get_tree_root();
        ImoObj* pRoot =  a.analyse_tree(tree, "string:");

//        cout << test_name() << endl;
//        cout << "[" << errormsg.str() << "]" << endl;
//        cout << "[" << expected.str() << "]" << endl;
        CHECK( errormsg.str() == expected.str() );
        CHECK( pRoot != nullptr );
        CHECK( pRoot && pRoot->is_document() == true );
        ImoDocument* pDoc = dynamic_cast<ImoDocument*>( pRoot );
        ImoScore* pScore = dynamic_cast<ImoScore*>( pDoc->get_content_item(0) );
        ImoInstrument* pInstr = pScore->get_instrument(0);
        TypeMeasureInfo* pInfo = pInstr->get_last_measure_info();
        CHECK( pInfo == nullptr );
        ImoMusicData* pMD = pInstr->get_musicdata();
        CHECK( pMD != nullptr );

        CHECK( pMD && pMD->get_num_children() == 7 );
        ImoObj::children_iterator it = pMD->begin();    //clef
        CHECK( (*it)->is_clef() );
        ++it;   //note c4
        CHECK( (*it)->is_note() );
        ++it;   //note e4
        CHECK( (*it)->is_note() );
        ++it;   //barline
        CHECK( (*it)->is_barline() );
        ImoBarline* pBarline = dynamic_cast<ImoBarline*>( *it );
        CHECK( pBarline != nullptr );
        CHECK( pBarline && pBarline->get_type() == k_barline_simple );
        CHECK( pBarline && pBarline->is_visible() );
        TypeMeasureInfo* pInfo1 = pBarline->get_measure_info();
        CHECK( pInfo1 != nullptr );
        CHECK( pInfo1->count == 1 );
        CHECK( pInfo1->number == "1" );
//        cout << test_name() << ": count=" << pInfo1->count
//             << ", number=" << pInfo1->number << endl;

        //measure 2
        ++it;   //note c4
        CHECK( (*it)->is_note() );
        ++it;   //note e4
        CHECK( (*it)->is_note() );
        ++it;   //barline
        CHECK( (*it)->is_barline() );
        pBarline = dynamic_cast<ImoBarline*>( *it );
        CHECK( pBarline != nullptr );
        CHECK( pBarline && pBarline->get_type() == k_barline_simple );
        CHECK( pBarline && pBarline->is_visible() );
        TypeMeasureInfo* pInfo2 = pBarline->get_measure_info();
        CHECK( pInfo2 != nullptr );
        CHECK( pInfo2->count == 2 );
        CHECK( pInfo2->number == "2" );
//        cout << test_name() << ": count=" << pInfo2->count
//             << ", number=" << pInfo2->number << endl;

        CHECK( pInfo1 != pInfo2 );

        if (pRoot && !pRoot->is_document()) delete pRoot;
    }


    //@ z. miscellaneous ----------------------------------------------------------------


    TEST_FIXTURE(MnxAnalyserTestFixture, MnxAnalyser_pitch_99001)
    {
        //@99001. pitch_to_components() method

        int step;
        int octave;
        EAccidentals acc;
        float alt;

        CHECK (MnxAnalyser::pitch_to_components("C4", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_C);
        CHECK (octave == 4);
        CHECK (acc == k_no_accidentals);
        CHECK (alt == 0.0f);

        CHECK (MnxAnalyser::pitch_to_components("C#4", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_C);
        CHECK (octave == 4);
        CHECK (acc == k_sharp);
        CHECK (alt == 0.0f);

        CHECK (MnxAnalyser::pitch_to_components("Db4", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_D);
        CHECK (octave == 4);
        CHECK (acc == k_flat);
        CHECK (alt == 0.0f);

        CHECK (MnxAnalyser::pitch_to_components("G3+0.5", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_G);
        CHECK (octave == 3);
        CHECK (acc == k_no_accidentals);
        CHECK (alt == 0.5f);

        CHECK (MnxAnalyser::pitch_to_components("B5+1.5", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_B);
        CHECK (octave == 5);
        CHECK (acc == k_no_accidentals);
        CHECK (alt == 1.5f);

        CHECK (MnxAnalyser::pitch_to_components("C4-0.5", &step, &octave, &acc, &alt) == false );
        CHECK (step == k_step_C);
        CHECK (octave == 4);
        CHECK (acc == k_no_accidentals);
        CHECK (alt == -0.5f);

        CHECK (MnxAnalyser::pitch_to_components("#c4", &step, &octave, &acc, &alt) == true );
        CHECK (MnxAnalyser::pitch_to_components("C+4", &step, &octave, &acc, &alt) == true );
        CHECK (MnxAnalyser::pitch_to_components("C12", &step, &octave, &acc, &alt) == true );
        CHECK (MnxAnalyser::pitch_to_components("C4##", &step, &octave, &acc, &alt) == true );
        CHECK (MnxAnalyser::pitch_to_components("C4+1,25", &step, &octave, &acc, &alt) == true );
    }

}

