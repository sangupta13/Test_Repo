//****************************************************************************
// (c) Copyright 1996-2004. Group 1 Software, Inc. All rights reserved.
//
// AGGREGATIONDLG.CPP
//
// DESCRIPTION:
//   Implements class CSaAnAggregationDlg.
//
//****************************************************************************

#include "an/StdAFX.h"
#include "sa/sa_defs.h"
#include "an/AggregationDlg.h"
#include "ClientSinkInterface/IClientSinkOperations.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//*****************************************************************************
//
// CSaAnAggregationDlg  MESSAGE_MAP
//
//*****************************************************************************

BEGIN_MESSAGE_MAP( CSaAnAggregationDlg, CSaAnAggregationDlgBase )
  //{{AFX_MSG_MAP(CSaAnAggregationDlg)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//*****************************************************************************
//
// CSaAnAggregationDlg::CSaAnAggregationDlg()
//
// DESCRIPTION:
//   Constructor.
//
//*****************************************************************************

CSaAnAggregationDlg::CSaAnAggregationDlg( CAnColumnManager         * pColMgr,
                                          ISaClientSinkOperations  * pCallBack,
                                          CWnd                     * pParent )
: CSaAnAggregationDlgBase( CSaAnAggregationDlg::IDD, pParent )
{
  m_iOldAggPlace = 0; // Coverity Fix For CID-3115<UNINIT_CTOR>

  //{{AFX_DATA_INIT(CSaAnAggregationDlg)
  m_iAggPlace = 0;
  //}}AFX_DATA_INIT

  ASSERT( pColMgr != NULL );
  m_pColMgr      = pColMgr;
  m_pCallBack    = pCallBack;
  m_bQueryChange = FALSE;     // On exit, this indicates if query was changed.
}


//*****************************************************************************
//
// CSaAnAggregationDlg::DoDataExchange()
//
//*****************************************************************************

void CSaAnAggregationDlg::DoDataExchange( CDataExchange *pDX )
{
  CSaAnAggregationDlgBase::DoDataExchange( pDX );
  //{{AFX_DATA_MAP(CSaAnAggregationDlg)
  DDX_Radio( pDX, IDC_AGG_LOCAL, m_iAggPlace );
  //}}AFX_DATA_MAP
}


//*****************************************************************************
//
// CSaAnAggregationDlg::OnInitDialog()
//
//*****************************************************************************

BOOL CSaAnAggregationDlg::OnInitDialog()
{
  CSaAnAggregationDlgBase::OnInitDialog();

  // Replace the placeholder with custom list box.
  CWnd *pLB = GetDlgItem( IDC_LIST );
  ASSERT( pLB != NULL );

  CRect rectLB;
  pLB->GetWindowRect( &rectLB );
  ScreenToClient( &rectLB );
  pLB->DestroyWindow();

  CString str;
  str.LoadString( IDS_SAAN_AGGDLG_MEASURE );
  m_clbMeasures.Create( rectLB, this, WM_NULL, str, FALSE );

  m_clbMeasures.ModifyStyle(0, WS_TABSTOP);
  m_clbMeasures.SetFocus();

  // Save this off to see if it changed later.
  m_iOldAggPlace = m_iAggPlace;

  // Send column choices to combo-listbox control.
  BuildFunctCollections();
  str.LoadString( IDS_SAAN_AGGDLG_FUNCTION );
  m_clbMeasures.AddColumn( str, &m_arrAggFunctionNames );

  // Disable if there are no measures.
  CArray<CAnColumn*,CAnColumn*> *pDataCols = m_pColMgr->GetDataArray();
  if ( pDataCols->GetSize() == 0 )
  {
    GetDlgItem( IDC_AGG_LOCAL )->EnableWindow( FALSE );
    GetDlgItem( IDC_AGG_SERVER )->EnableWindow( FALSE );
    GetDlgItem( IDOK )->EnableWindow( FALSE );
    m_clbMeasures.EnableWindow( FALSE );
  }
  else
  {
    // Now add items - one for each data field.
    BOOL bFoundServerOnlyAggregateable = FALSE;
    for ( int i=0; i<pDataCols->GetSize(); i++ )
    {
      CAnColumn *pCol = pDataCols->GetAt(i);
      ASSERT( pCol != NULL );

      m_clbMeasures.AddItem( pCol->m_strFieldName );

      // Set the choice to the current function type for this measure.
      int iChoice = -1;
      m_mapAggFunctToIndex.Lookup( pCol->m_eAggFunct, iChoice );
      m_clbMeasures.SetColumnChoice( 1, i, iChoice );

      // Prevent the user from changing function of private columns.
      if ( pCol->m_bPrivate )
        m_clbMeasures.EnableItem( 1, i, FALSE );

      // Prevent the user from changing function if aggregated on
      // server and aggregation cannot be changed.
      if ( !pCol->m_bNoServerAgg  &&  !pCol->m_bCanChangeAggregation )
      {
        m_clbMeasures.EnableItem( 1, i, FALSE );
        bFoundServerOnlyAggregateable = TRUE;
      }
    }

    // Disable the server/client radio buttons if found column
    // for which server aggregation cannot be removed or if we
    // have no callback.  The callback pointer will be NULL if changes
    // to the plan are not allowed.
    if ( bFoundServerOnlyAggregateable  ||  m_pCallBack == NULL )
    {
      GetDlgItem( IDC_AGG_LOCAL )->EnableWindow( FALSE );
      GetDlgItem( IDC_AGG_SERVER )->EnableWindow( FALSE );
    }

    // Disable listbox if server-side agg and we have no callback.  (Since
    // we have no way of changing aggregation.)  This dialog should
    // really not be called for this case, but we check here just in case.
    if ( m_iAggPlace != 0  &&  m_pCallBack == NULL )
      m_clbMeasures.EnableWindow(FALSE);

    // Select first item in list.
    m_clbMeasures.SetSelectedItem(0);
    UpdateData(FALSE);
  }

  return  TRUE;  // Return TRUE unless you set the focus to a control.
}


//****************************************************************************
//
// CSaAnAggregationDlg::BuildFunctCollections()
//
// DESCRIPTION:
//   This private function is called to construct our internal collections
//   used to maintain mappings between the aggregate function type names
//   passed to the combo-list control, and the enumerated values that are
//   used in each CAnColumn object.
//
//****************************************************************************

void CSaAnAggregationDlg::BuildFunctCollections()
{
  CString strName;
  strName.LoadString(IDS_AGG_AVG);
  m_mapAggFunctToIndex.SetAt(SACLIENTSINK_FUNCTYPE_AVG,
    m_arrIndexToAggFunct.Add(SACLIENTSINK_FUNCTYPE_AVG));
  m_arrAggFunctionNames.Add(strName);

  strName.LoadString(IDS_AGG_COUNT);
  m_mapAggFunctToIndex.SetAt(SACLIENTSINK_FUNCTYPE_COUNT,
    m_arrIndexToAggFunct.Add(SACLIENTSINK_FUNCTYPE_COUNT));
  m_arrAggFunctionNames.Add(strName);

  strName.LoadString(IDS_AGG_MAX);
  m_mapAggFunctToIndex.SetAt(SACLIENTSINK_FUNCTYPE_MAX,
    m_arrIndexToAggFunct.Add(SACLIENTSINK_FUNCTYPE_MAX));
  m_arrAggFunctionNames.Add(strName);

  strName.LoadString(IDS_AGG_MIN);
  m_mapAggFunctToIndex.SetAt(SACLIENTSINK_FUNCTYPE_MIN,
    m_arrIndexToAggFunct.Add(SACLIENTSINK_FUNCTYPE_MIN));
  m_arrAggFunctionNames.Add(strName);

  strName.LoadString(IDS_AGG_SUM);
  m_mapAggFunctToIndex.SetAt(SACLIENTSINK_FUNCTYPE_SUM,
    m_arrIndexToAggFunct.Add(SACLIENTSINK_FUNCTYPE_SUM));
  m_arrAggFunctionNames.Add(strName);
}


//****************************************************************************
//
// CSaAnAggregationDlg::OnOK()
//
// DESCRIPTION:
//   Handles OK button. We save the new settings into the columns here.
//
//****************************************************************************

void CSaAnAggregationDlg::OnOK()
{
  if ( UpdateData(TRUE) )
  {
    CWaitCursor waitCursor;

    CArray<CAnColumn*,CAnColumn*> *pDataCols = m_pColMgr->GetDataArray();
    for ( int i=0; i<pDataCols->GetSize(); i++ )
    {
      CAnColumn *pCol = pDataCols->GetAt(i);

      //  Get the funct type chosen for this measure.
      SACLIENTSINK_FUNCTYPE eFunct =
        m_arrIndexToAggFunct.GetAt( m_clbMeasures.GetColumnChoice(1,i) );

      // If the agg. place changed, must add or remove column aggregate
      // in the query accordingly.
      if ( m_iOldAggPlace != m_iAggPlace )
      {
        pCol->m_eAggFunct = eFunct;

        if ( m_iAggPlace == 0 )
        {
          // Switched to client side agg, so remove column aggregate funct.
          if ( m_pCallBack && pCol->m_bCanChangeAggregation )
          {
            HRESULT hr = m_pCallBack->PlanQueryStep_RemoveColumnAggregate(
                                                      pCol->m_strDatasetId );
            ASSERT( hr == S_OK );
            m_bQueryChange = TRUE;   // Let caller know query has changed.
          }
        }
        else
        {
          // Switched to server side agg, so add column aggregate funct.
          if ( m_pCallBack && pCol->m_bCanChangeAggregation )
          {
            CString strNewID;
            HRESULT hr = m_pCallBack->PlanQueryStep_ReplaceColumnWithAggregate(
                            pCol->m_strDatasetId, pCol->m_eAggFunct, strNewID );
            ASSERT( hr == S_OK );
            if ( hr == S_OK )
              m_pColMgr->ChangeID( pCol->m_strDatasetId, strNewID );
            m_bQueryChange = TRUE;   // Let caller know query has changed.
          }
        }
      }
      else
      {
        // Agg place didn't change, but if it's on the server side AND the
        // funct type changed, must still do a column replacement.
        if ( pCol->m_eAggFunct != eFunct  &&  m_iAggPlace == 1 )
        {
          if ( m_pCallBack  &&  pCol->m_bCanChangeAggregation )
          {
            HRESULT hr = m_pCallBack->PlanQueryStep_ChangeColumnAggregate(
                                             pCol->m_strDatasetId, eFunct );
            ASSERT( hr == S_OK );
            m_bQueryChange = TRUE;   // Let caller know query has changed.
          }
        }

        // Update this in any case (after the above comparison check).
        pCol->m_eAggFunct = eFunct;
      }
    }

    EndDialog( IDOK );
  }
}
