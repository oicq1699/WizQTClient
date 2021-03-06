#include "wizapi.h"

#include <QFile>

#include "share/wizzip.h"
#include "wizsettings.h"
#include "wizmisc.h"

#include "wizApiEntry.h"


CWizApiBase::CWizApiBase(const QString& strKbUrl /* = WIZ_API_URL*/ , QObject* parent /* = 0 */)
    : m_nCurrentObjectAllSize(0)
    , m_bDownloadingObject(false)
    , QObject(parent)
{
    qRegisterMetaType<CWizGroupDataArray>("CWizGroupDataArray");

    m_server = new CWizXmlRpcServer(strKbUrl, this);

    connect(m_server, SIGNAL(xmlRpcReturn(CWizXmlRpcValue&, const QString&, const QString&, const QString&)),
            SLOT(xmlRpcReturn(CWizXmlRpcValue&, const QString&, const QString&, const QString&)));

    connect(m_server, SIGNAL(xmlRpcError(const QString&, WizXmlRpcError, int, const QString&)), \
            SLOT(xmlRpcError(const QString&, WizXmlRpcError, int, const QString&)));

    resetProxy();
}

bool CWizApiBase::callXmlRpc(CWizXmlRpcValue* pVal,
                             const QString& strMethodName,
                             const QString& arg1 /* = "" */,
                             const QString& arg2 /* = "" */)
{
    return m_server->xmlRpcCall(pVal, strMethodName, arg1, arg2);
}

void CWizApiBase::xmlRpcReturn(CWizXmlRpcValue& ret, const QString& strMethodName,
                               const QString& arg1, const QString& arg2)
{
    onXmlRpcReturn(ret, strMethodName, arg1, arg2);
}

void CWizApiBase::xmlRpcError(const QString& strMethodName, WizXmlRpcError err, int errorCode, const QString& errorMessage)
{
    _onXmlRpcError(strMethodName, err, errorCode, errorMessage);
}

void CWizApiBase::onXmlRpcReturn(CWizXmlRpcValue& ret, const QString& strMethodName,
                                 const QString& arg1, const QString& arg2)
{
    Q_UNUSED(arg2);
    if (strMethodName == SyncMethod_ClientLogin)
    {
        //qDebug() << "\n[XML-RPC]accounts.clientLogin:\n" << ret.ToString() << "\n\n";

        WIZUSERINFO userInfo;

        // pass empty kbguid here is ok
        if (!ret.ToData<WIZUSERINFO>(userInfo)) {
            return;
        }

        // save user info
        WizGlobal()->setUserInfo(userInfo);

        // set kbguid
        setKbGUID(userInfo.strKbGUID);

        onClientLogin(userInfo);

        Q_EMIT clientLoginDone();
    }
    /*

    else if (strMethodName == SyncMethod_ClientKeepAlive)
    {
        // set kbguid
        setKbGUID(WizGlobal()->userInfo().strKbGUID);

        onClientLogin(WizGlobal()->userInfo());

        Q_EMIT clientLoginDone();

    }
    else if (strMethodName == SyncMethod_ClientLogout)
    {
        onClientLogout();

        Q_EMIT clientLogoutDone();
    }
    else if (strMethodName == SyncMethod_CreateAccount)
    {
        onCreateAccount();
    }
    else if (strMethodName == SyncMethod_GetUserInfo)
    {
        onGetUserInfo(ret);
    }
    else if (strMethodName == SyncMethod_GetUserCert)
    {
        WIZUSERCERT data;
        ret.ToData(data, kbGUID());
        onGetUserCert(data);
    }
    else if (strMethodName == SyncMethod_GetGroupList)
    {
        //qDebug() << "\n[XML-RPC]accounts.getGroupKbList:\n" << ret.ToString() << "\n\n";

        std::deque<WIZGROUPDATA> arrayGroup;
        ret.ToArray<WIZGROUPDATA>(arrayGroup, kbGUID());
        onGetGroupList(arrayGroup);

        Q_EMIT getGroupListDone(arrayGroup);
    }
    else if (strMethodName == SyncMethod_GetMessages)
    {
        //qDebug() << "\n[XML-RPC]accounts.getMessages:\n" << ret.ToString() << "\n\n";

        CWizMessageDataArray arrayData;
        ret.ToArray<WIZMESSAGEDATA>(arrayData, kbGUID());
        onGetMessages(arrayData);
    }
    else if (strMethodName == SyncMethod_SetMessageStatus)
    {
        qDebug() << "\n[XML-RPC]accounts.setReadStatus:\n" << ret.ToString() << "\n\n";

        onSetMessageStatus();
    }
    else if (strMethodName == SyncMethod_GetValue)
    {
        qDebug() << "\n[XML-RPC]accounts.getValue:\n" << ret.ToString() << "\n\n";

        WIZKVRETURN kv;
        ret.ToData<WIZKVRETURN>(kv, kbGUID());

        if (arg1.toInt() == KVTypeUserAlias) {
            onGetBizUsers(kv.value);
        } else {
            Q_ASSERT(0);
        }
    }
    else if (strMethodName == SyncMethod_SetValue)
    {

    }
    else if (strMethodName == SyncMethod_KbGetValueVersion)
    {
        //qDebug() << "\n[XML-RPC]kb.getValueVersion:\n" << ret.ToString() << "\n\n";

        WIZKVRETURN kv;
        ret.ToData<WIZKVRETURN>(kv, kbGUID());
        if (arg1.toInt() == KVTypeFolders) {
            Q_EMIT folderGetVersionDone(kv.nVersion);
        } else {
            Q_ASSERT(0);
        }
    }
    else if (strMethodName == SyncMethod_KbGetValue)
    {
        qDebug() << "\n[XML-RPC]kb.getValue:\n" << ret.ToString() << "\n\n";

        WIZKVRETURN kv;
        ret.ToData<WIZKVRETURN>(kv, kbGUID());

        if (arg1.toInt() == KVTypeFolders) {
            QStringList listFolder = kv.value.split("*", QString::SkipEmptyParts);
            Q_EMIT folderGetListDone(listFolder, kv.nVersion);
        } else {
            Q_ASSERT(0);
        }
    }
    else if (strMethodName == SyncMethod_KbSetValue)
    {
        qDebug() << "\n[XML-RPC]kb.setValue:\n" << ret.ToString() << "\n\n";

        WIZKVRETURN kv;
        ret.ToData<WIZKVRETURN>(kv, kbGUID());

        if (arg1.toInt() == KVTypeFolders) {
            Q_EMIT folderPostListDone(kv.nVersion);
        } else {
            Q_ASSERT(0);
        }
    }
    // deleteds
    else if (strMethodName == SyncMethod_GetDeletedList)
    {
        std::deque<WIZDELETEDGUIDDATA> arrayData;
        ret.ToArray<WIZDELETEDGUIDDATA>(arrayData, kbGUID());
        onDeletedGetList(arrayData);
    }
    else if (strMethodName == SyncMethod_PostDeletedList)
    {
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            CWizDeletedGUIDDataArray arrayDeleted;
            onDeletedPostList(arrayDeleted);
        } else {
            onDeletedPostList(m_arrayCurrentPostDeletedGUID);
        }
    }
    // tags
    else if (strMethodName == SyncMethod_GetTagList)
    {
        std::deque<WIZTAGDATA> arrayData;
        ret.ToArray<WIZTAGDATA>(arrayData, kbGUID());
        onTagGetList(arrayData);
    }
    else if (strMethodName == SyncMethod_PostTagList)
    {
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            CWizTagDataArray arrayTag;
            onTagPostList(arrayTag);
        } else {
            onTagPostList(m_arrayCurrentPostTag);
        }
    }
    // styles
    else if (strMethodName == SyncMethod_GetStyleList)
    {
        CWizStyleDataArray arrayData;
        ret.ToArray<WIZSTYLEDATA>(arrayData, kbGUID());
        onStyleGetList(arrayData);
    }
    else if (strMethodName == SyncMethod_PostStyleList)
    {
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            CWizStyleDataArray arrayStyle;
            onStylePostList(arrayStyle);
        } else {
            onStylePostList(m_arrayCurrentPostStyle);
        }
    }
    // documents
    else if (strMethodName == SyncMethod_GetDocumentList)
    {
        //qDebug() << "\n[XML-RPC]document.getList:\n" << ret.ToString() << "\n\n";

        std::deque<WIZDOCUMENTDATABASE> arrayData;
        ret.ToArray<WIZDOCUMENTDATABASE>(arrayData, kbGUID());
        onDocumentGetList(arrayData);
    }
    else if (strMethodName == SyncMethod_GetDocumentsInfo)
    {
        std::deque<WIZDOCUMENTDATABASE> arrayData;
        ret.ToArray<WIZDOCUMENTDATABASE>(arrayData, kbGUID());
        onDocumentsGetInfo(arrayData);
    }
    else if (strMethodName == SyncMethod_GetDocumentFullInfo)
    {
        //qDebug() << "\n[XML-RPC]document.getData:\n" << ret.ToString() << "\n\n";

        WIZDOCUMENTDATAEX data;
        ret.ToData(data, kbGUID());
        onDocumentGetData(data);
    }
    else if (strMethodName == SyncMethod_PostDocumentData)
    {
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            WIZDOCUMENTDATAEX doc;
            onDocumentPostData(doc);
        } else {
            onDocumentPostData(m_currentDocument);
        }
    }
    // attachments
    else if (strMethodName == SyncMethod_GetAttachmentList)
    {
        std::deque<WIZDOCUMENTATTACHMENTDATAEX> arrayData;
        ret.ToArray<WIZDOCUMENTATTACHMENTDATAEX>(arrayData, kbGUID());
        onAttachmentGetList(arrayData);
    }
    else if (strMethodName == SyncMethod_GetAttachmentsInfo)
    {
        std::deque<WIZDOCUMENTATTACHMENTDATAEX> arrayData;
        ret.ToArray<WIZDOCUMENTATTACHMENTDATAEX>(arrayData, kbGUID());
        onAttachmentsGetInfo(arrayData);
    }
    else if (strMethodName == SyncMethod_PostAttachmentData)
    {
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            WIZDOCUMENTATTACHMENTDATAEX data;
            onAttachmentPostData(data);
        } else {
            onAttachmentPostData(m_currentAttachment);
        }
    }
    // trunk data
    else if (strMethodName == SyncMethod_DownloadObjectPart)
    {
        //qDebug() << "\n[XML-RPC]data.download:\n" << ret.ToString() << "\n\n";

        WIZOBJECTPARTDATA data;
        CWizXmlRpcFaultValue* pFault = dynamic_cast<CWizXmlRpcFaultValue *>(&ret);
        if (pFault) {
            onDownloadDataPart(data);
        } else {
            data = m_currentObjectPartData;
            ret.ToData(data, kbGUID());
            onDownloadDataPart(data);
        }
    }
    else if (strMethodName == SyncMethod_UploadObjectPart)
    {
        onUploadDataPart();
    }
    else
    {
        Q_ASSERT(0);
    }
    */
}

void CWizApiBase::_onXmlRpcError(const QString& strMethodName,
                                 WizXmlRpcError err,
                                 int errorCode,
                                 const QString& errorMessage)
{
    // token timeout
    if (strMethodName == SyncMethod_ClientKeepAlive && err == errorXmlRpcFault) {
        TOLOG(tr("Token expired, Try to request another"));
        WizGlobal()->clearToken();
        callClientLogin(m_strUserId, m_strPasswd);
        return;
    }

    if (err == errorNetwork) {
        WizGlobal()->clearToken();
    }

    onXmlRpcError(strMethodName, err, errorCode, errorMessage);
}

void CWizApiBase::onXmlRpcError(const QString& strMethodName, WizXmlRpcError err, int errorCode, const QString& errorMessage)
{
    Q_UNUSED(err);
    Q_UNUSED(errorCode);

    QString errorMsg(QString("Error: [%1]: %2").arg(strMethodName).arg(errorMessage));
    Q_EMIT processErrorLog(errorMsg);
}

void CWizApiBase::resetProxy()
{
    CWizSettings settings(WizGetSettingsFileName());

    bool bStatus = settings.GetProxyStatus();

    if (bStatus) {
        QString host = settings.GetProxyHost();
        int port = settings.GetProxyPort();
        QString userName = settings.GetProxyUserName();
        QString password = settings.GetProxyPassword();

        m_server->setProxy(host, port, userName, password);
    } else {
        m_server->setProxy(0, 0, 0, 0);
    }
}

CString CWizApiBase::MakeXmlRpcUserId(const CString& strUserId)
{
    return strUserId;
}

CString CWizApiBase::MakeXmlRpcPassword(const CString& strPassword)
{
    return "md5." + ::WizMd5StringNoSpaceJava(strPassword.toUtf8());
}

bool CWizApiBase::callClientLogin(const QString& strUserId, const QString& strPassword)
{
    m_strUserId = strUserId;
    m_strPasswd = strPassword;

    // fetch api url first
    if (WizGlobal()->apiUrl().isEmpty()) {
        qDebug() << "[WIZAPI]api entry is empty, acquire entry...";

        CWizApiEntry* entry = new CWizApiEntry(this);
        connect(entry, SIGNAL(acquireEntryFinished(const QString&)),
                SLOT(on_acquireApiEntry_finished(const QString&)));
        entry->getSyncUrl();
        return true;
    } else {
        setKbUrl(WizGlobal()->apiUrl());
    }

    if (WizGlobal()->token().isEmpty()) {
        CWizApiParamBase param;
        param.AddString("user_id", MakeXmlRpcUserId(m_strUserId));
        param.AddString("password", MakeXmlRpcPassword(m_strPasswd));

        return callXmlRpc(&param, SyncMethod_ClientLogin);
    } else {
        return callClientKeepAlive();
    }
}

void CWizApiBase::on_acquireApiEntry_finished(const QString& strReply)
{
    sender()->deleteLater();

    // if fetch api entry failed, use default one
    if (strReply.isEmpty()) {
        qDebug() << "[WIZAPI]failed: unable to acquire api entry!";
    }

    WizGlobal()->setApiUrl(strReply);

    qDebug() << "[WIZAPI]acquire entry finished, url: " << strReply;

    callClientLogin(m_strUserId, m_strPasswd);
}

bool CWizApiBase::callClientKeepAlive()
{
    CWizApiParamBase param;
    param.AddString("token", WizGlobal()->token());

    return callXmlRpc(&param, SyncMethod_ClientKeepAlive);
}

bool CWizApiBase::callClientLogout()
{
    CWizApiTokenParam param(*this);
    return callXmlRpc(&param, SyncMethod_ClientLogout);
}

void CWizApiBase::onClientLogout()
{
}

bool CWizApiBase::callGetUserInfo()
{
    return true;
}

bool CWizApiBase::callGetUserCert(const QString& strUserId, const QString& strPassword)
{
    CWizApiParamBase param;
    param.AddString("user_id", MakeXmlRpcUserId(strUserId));
    param.AddString("password", MakeXmlRpcPassword(strPassword));

    return callXmlRpc(&param, SyncMethod_GetUserCert);
}

bool CWizApiBase::callCreateAccount(const CString& strUserId, const CString& strPassword)
{
    CWizApiParamBase param;
    param.AddString("user_id", MakeXmlRpcUserId(strUserId));
    param.AddString("password", MakeXmlRpcPassword(strPassword));

#if defined Q_OS_MAC
    param.AddString("invite_code", "129ce11c");
    param.AddString("product_name", "qtMac");
#elif defined Q_OS_LINUX
    param.AddString("invite_code", "7abd8f4a");
    param.AddString("product_name", "qtLinux");
#else
    param.AddString("invite_code", "8480c6d7");
    param.AddString("product_name", "qtWindows");
#endif

    return callXmlRpc(&param, SyncMethod_CreateAccount);
}

bool CWizApiBase::callGetGroupList()
{
    CWizApiTokenParam param(*this);
    return callXmlRpc(&param, SyncMethod_GetGroupList);
}

bool CWizApiBase::callGetBizUsers(const QString& bizGUID)
{
    // special case, no kb_guid passed as filter
    CWizApiParamBase param;
    param.AddString("token", WizGlobal()->token());
    param.AddString("key", QString(WIZAPI_KV_KEY_USER_ALIAS).arg(bizGUID));
    return callXmlRpc(&param, SyncMethod_GetValue, QString::number(KVTypeUserAlias));
}

bool CWizApiBase::callFolderGetVersion()
{
    CWizApiTokenParam param(*this);
    param.AddString("key", WIZAPI_KV_KEY_FOLDERS);
    return callXmlRpc(&param, SyncMethod_KbGetValueVersion, QString::number(KVTypeFolders));
}

bool CWizApiBase::callFolderGetList()
{
    CWizApiTokenParam param(*this);
    param.AddString("key", WIZAPI_KV_KEY_FOLDERS);
    return callXmlRpc(&param, SyncMethod_KbGetValue, QString::number(KVTypeFolders));
}

bool CWizApiBase::callFolderPostList(const CWizStdStringArray& arrayFolder)
{
    if (!arrayFolder.size()) {
        return false;
    }

    QString strFolders = "*";

    if (arrayFolder.size() >= 2) {
        for(int i = 0; i < arrayFolder.size() - 1; i++) {
            strFolders = strFolders + arrayFolder.at(i) + "*";
        }
    }
    strFolders += arrayFolder.back();

    CWizApiTokenParam param(*this);
    param.AddString("key", WIZAPI_KV_KEY_FOLDERS);
    param.AddString("value_of_key", strFolders);
    return callXmlRpc(&param, SyncMethod_KbSetValue, QString::number(KVTypeFolders));
}

bool CWizApiBase::callGetMessages(qint64 nVersion)
{
    CWizApiParamBase param;
    param.AddString("token", WizGlobal()->token());
    param.AddString("version", WizInt64ToStr(nVersion));

    return callXmlRpc(&param, SyncMethod_GetMessages);
}

bool CWizApiBase::callSetMessageStatus(const QList<qint64>& ids, bool bRead)
{
    if (!ids.size()) {
        return false;
    }

    QString msgs;

    if (ids.size() >= 2) {
        for (int i = 0; i < ids.size() - 1; i++) {
            QString strId = QString::number(ids.at(i));
            msgs += (strId + ",");
        }
    }

    msgs += QString::number(ids.last());

    CWizApiParamBase param;
    param.AddString("token", WizGlobal()->token());
    param.AddString("ids", msgs);
    param.AddString("status", bRead ? "1" : "0");

    return callXmlRpc(&param, SyncMethod_SetMessageStatus);
}

bool CWizApiBase::callDeletedGetList(qint64 nVersion)
{
    return callGetList(SyncMethod_GetDeletedList, nVersion);
}

bool CWizApiBase::callDeletedPostList(const std::deque<WIZDELETEDGUIDDATA>& arrayData)
{
    m_arrayCurrentPostDeletedGUID.assign(arrayData.begin(), arrayData.end());
    return callPostList(SyncMethod_PostDeletedList, "deleteds", arrayData);
}

bool CWizApiBase::callTagGetList(qint64 nVersion)
{
    return callGetList(SyncMethod_GetTagList, nVersion);
}

bool CWizApiBase::callTagPostList(const std::deque<WIZTAGDATA>& arrayData)
{
    m_arrayCurrentPostTag.assign(arrayData.begin(), arrayData.end());
    return callPostList(SyncMethod_PostTagList, "tags", arrayData);
}

bool CWizApiBase::callStyleGetList(qint64 nVersion)
{
    return callGetList(SyncMethod_GetStyleList, nVersion);
}

bool CWizApiBase::callStylePostList(const std::deque<WIZSTYLEDATA>& arrayData)
{
    m_arrayCurrentPostStyle.assign(arrayData.begin(), arrayData.end());
    return callPostList(SyncMethod_PostStyleList, "styles", arrayData);
}

bool CWizApiBase::callDocumentGetList(qint64 nVersion)
{
    return callGetList(SyncMethod_GetDocumentList, nVersion);
}

bool CWizApiBase::callDocumentGetInfo(const QString& documentGUID)
{
    CWizStdStringArray arrayDocumentGUID;
    arrayDocumentGUID.push_back(documentGUID);
    return callDocumentsGetInfo(arrayDocumentGUID);
}

bool CWizApiBase::callDocumentsGetInfo(const CWizStdStringArray& arrayDocumentGUID)
{
    CWizApiTokenParam param(*this);
    param.AddStringArray("document_guids", arrayDocumentGUID);

    return callXmlRpc(&param, SyncMethod_GetDocumentsInfo);
}

bool CWizApiBase::callDocumentGetData(const QString& documentGUID)
{
    CWizApiTokenParam param(*this);
    param.AddString("document_guid", documentGUID);
    param.AddBool("document_info",  true);
    param.AddBool("document_param", true);

    return callXmlRpc(&param, SyncMethod_GetDocumentFullInfo);
}

bool CWizApiBase::callDocumentGetData(const WIZDOCUMENTDATABASE& data)
{
    int nPart = data.nObjectPart;
    Q_ASSERT(nPart != 0);

    CWizApiTokenParam param(*this);
    param.AddString("document_guid", data.strGUID);
    param.AddBool("document_info", (nPart & WIZKM_XMKRPC_DOCUMENT_PART_INFO) ? true : false);
    param.AddBool("document_param", (nPart & WIZKM_XMKRPC_DOCUMENT_PART_PARAM) ? true : false);

    return callXmlRpc(&param, SyncMethod_GetDocumentFullInfo);
}

bool CWizApiBase::callDocumentPostData(const WIZDOCUMENTDATAEX& data)
{
    m_currentDocument = data;

    int nParts = m_currentDocument.nObjectPart;
    Q_ASSERT(0 != nParts);
    bool bInfo = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_INFO) ? true : false;
    bool bParam = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_PARAM) ? true : false;
    bool bData = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_DATA) ? true : false;

    QString info = data.strTitle;
    Q_EMIT processLog(tr("upload document info:") + info);

    CWizApiTokenParam param(*this);

    CWizXmlRpcStructValue* pDocumentStruct = new CWizXmlRpcStructValue();
    param.AddStruct(_T("document"), pDocumentStruct);

    pDocumentStruct->AddString(_T("document_guid"), data.strGUID);
    pDocumentStruct->AddBool(_T("document_info"), bInfo ? true : false);
    pDocumentStruct->AddBool(_T("document_data"), bData ? true : false);
    pDocumentStruct->AddBool(_T("document_param"), bParam ? true : false);

    bool bParamInfoAdded = false;
    bool bDataInfoAdded = false;

    const WIZDOCUMENTDATAEX& infodata = data; //m_currentDocument;

    if (bInfo)
    {
        pDocumentStruct->AddString(_T("document_title"), infodata.strTitle);
        pDocumentStruct->AddString(_T("document_category"), infodata.strLocation);
        pDocumentStruct->AddString(_T("document_filename"), infodata.strName);
        pDocumentStruct->AddString(_T("document_seo"), infodata.strSEO);
        pDocumentStruct->AddString(_T("document_url"), infodata.strURL);
        pDocumentStruct->AddString(_T("document_author"), infodata.strAuthor);
        pDocumentStruct->AddString(_T("document_keywords"), infodata.strKeywords);
        pDocumentStruct->AddString(_T("document_type"), infodata.strType);
        pDocumentStruct->AddString(_T("document_owner"), infodata.strOwner);
        pDocumentStruct->AddString(_T("document_filetype"), infodata.strFileType);
        pDocumentStruct->AddString(_T("document_styleguid"), infodata.strStyleGUID);
        pDocumentStruct->AddTime(_T("dt_created"), infodata.tCreated);
        pDocumentStruct->AddTime(_T("dt_modified"), infodata.tModified);
        pDocumentStruct->AddTime(_T("dt_accessed"), infodata.tAccessed);
        pDocumentStruct->AddInt(_T("document_iconindex"), infodata.nIconIndex);
        pDocumentStruct->AddInt(_T("document_protected"), infodata.nProtected);
        pDocumentStruct->AddInt(_T("document_readcount"), infodata.nReadCount);
        pDocumentStruct->AddInt(_T("document_attachment_count"), infodata.nAttachmentCount);
        pDocumentStruct->AddTime(_T("dt_info_modified"), infodata.tInfoModified);
        pDocumentStruct->AddString(_T("info_md5"), infodata.strInfoMD5);
        pDocumentStruct->AddTime(_T("dt_data_modified"), infodata.tDataModified);
        pDocumentStruct->AddString(_T("data_md5"), infodata.strDataMD5);
        pDocumentStruct->AddTime(_T("dt_param_modified"), infodata.tParamModified);
        pDocumentStruct->AddString(_T("param_md5"), infodata.strParamMD5);
        pDocumentStruct->AddString(_T("system_tags"), infodata.strSystemTags);
        pDocumentStruct->AddInt(_T("document_share"), infodata.nShareFlags);

        //m_db->GetDocumentTags(infodata.strGUID, infodata.arrayTagGUID);
        pDocumentStruct->AddStringArray(_T("document_tags"), infodata.arrayTagGUID);

        bParamInfoAdded = true;
        bDataInfoAdded = true;
    }

    if (bParam)
    {
        //m_db->GetDocumentParams(infodata.strGUID, infodata.arrayParam);

        if (!bParamInfoAdded)
        {
            pDocumentStruct->AddTime(_T("dt_param_modified"), infodata.tParamModified);
            pDocumentStruct->AddString(_T("param_md5"), infodata.strParamMD5);
            bParamInfoAdded = true;
        }

        pDocumentStruct->AddArray(_T("document_params"), infodata.arrayParam);
    }

    if (bData)
    {
        //if (!m_db->LoadDocumentData(m_currentDocument.strGUID, m_currentDocument.arrayData))
        //{
        //    //skip this document
        //    QString info2 = data.strTitle;
        //    Q_EMIT processErrorLog(tr("Can not load document data: ") + info2);
        //    onUploadObjectDataCompleted(data);
        //    return false;
        //}

        if (!bDataInfoAdded)
        {
            pDocumentStruct->AddTime(_T("dt_data_modified"), infodata.tDataModified);
            pDocumentStruct->AddString(_T("data_md5"), infodata.strDataMD5);
            bDataInfoAdded = true;
        }

        pDocumentStruct->AddString(_T("document_zip_md5"), WizMd5StringNoSpaceJava(infodata.arrayData));
    }

    return callXmlRpc(&param, SyncMethod_PostDocumentData);
}

bool CWizApiBase::callAttachmentGetList(qint64 nVersion)
{
    return callGetList(SyncMethod_GetAttachmentList, nVersion);
}

bool CWizApiBase::callAttachmentGetInfo(const QString& attachmentGUID)
{
    CWizStdStringArray arrayGUID;
    arrayGUID.push_back(attachmentGUID);

    return callAttachmentsGetInfo(arrayGUID);
}

bool CWizApiBase::callAttachmentsGetInfo(const CWizStdStringArray& arrayAttachmentGUID)
{
    CWizApiTokenParam param(*this);
    param.AddStringArray("attachment_guids", arrayAttachmentGUID);

    return callXmlRpc(&param, SyncMethod_GetAttachmentsInfo);
}

bool CWizApiBase::callAttachmentPostData(const WIZDOCUMENTATTACHMENTDATAEX& data)
{
    m_currentAttachment = data;

    int nParts = data.nObjectPart;
    Q_ASSERT(0 != nParts);

    QString info = data.strName;
    Q_EMIT processLog(tr("upload attachment info: ") + info);

    CWizApiTokenParam param(*this);

    CWizXmlRpcStructValue* pAttachmentStruct = new CWizXmlRpcStructValue();
    param.AddStruct(_T("attachment"), pAttachmentStruct);

    bool bInfo = (nParts & WIZKM_XMKRPC_ATTACHMENT_PART_INFO) ? true : false;
    bool bData = (nParts & WIZKM_XMKRPC_ATTACHMENT_PART_DATA) ? true : false;

    pAttachmentStruct->AddString(_T("attachment_guid"), data.strGUID);
    pAttachmentStruct->AddBool(_T("attachment_info"), bInfo ? true : false);
    pAttachmentStruct->AddBool(_T("attachment_data"), bData ? true : false);

    bool bDataInfoAdded = false;

    const WIZDOCUMENTATTACHMENTDATAEX& infodata = data;

    if (bInfo)
    {
        pAttachmentStruct->AddString(_T("attachment_document_guid"), infodata.strDocumentGUID);
        pAttachmentStruct->AddString(_T("attachment_name"), infodata.strName);
        pAttachmentStruct->AddString(_T("attachment_url"), infodata.strURL);
        pAttachmentStruct->AddString(_T("attachment_description"), infodata.strDescription);
        pAttachmentStruct->AddTime(_T("dt_info_modified"), infodata.tInfoModified);
        pAttachmentStruct->AddString(_T("info_md5"), infodata.strInfoMD5);
        pAttachmentStruct->AddTime(_T("dt_data_modified"), infodata.tDataModified);
        pAttachmentStruct->AddString(_T("data_md5"), infodata.strDataMD5);

        bDataInfoAdded = true;
    }

    if (bData)
    {
        if (!bDataInfoAdded)
        {
            pAttachmentStruct->AddTime(_T("dt_data_modified"), infodata.tDataModified);
            pAttachmentStruct->AddString(_T("data_md5"), infodata.strDataMD5);
            bDataInfoAdded = true;
        }
        pAttachmentStruct->AddString(_T("attachment_zip_md5"), WizMd5StringNoSpaceJava(infodata.arrayData));
    }

    return callXmlRpc(&param, SyncMethod_PostAttachmentData);
}

// download trunk data
bool CWizApiBase::downloadObjectData(const WIZOBJECTDATA& data)
{
    if (data.eObjectType == wizobjectDocument)
    {
        QString info = data.strDisplayName;
        Q_EMIT processLog(tr("downloading note: ") + info);
    }
    else if (data.eObjectType == wizobjectDocumentAttachment)
    {
        QString info = data.strDisplayName;
        Q_EMIT processLog(tr("downloading attachment: ") + info);
    }
    else
    {
        Q_ASSERT(false);
    }

    Q_EMIT progressChanged(0);

    m_currentObjectData = data;
    m_currentObjectData.arrayData.clear();;

    return downloadNextPartData();
}

bool CWizApiBase::downloadNextPartData()
{
    int size = m_currentObjectData.arrayData.size();
    return callDownloadDataPart(m_currentObjectData.strObjectGUID,
                                WIZOBJECTDATA::ObjectTypeToTypeString(m_currentObjectData.eObjectType),
                                size);
}

bool CWizApiBase::callDownloadDataPart(const CString& strObjectGUID, const CString& strObjectType, int pos)
{
    m_bDownloadingObject = true;

    unsigned int size = WIZAPI_TRUNK_SIZE;

    m_currentObjectPartData.strObjectGUID = strObjectGUID;
    m_currentObjectPartData.strObjectType = strObjectType;
    m_currentObjectPartData.nStartPos = pos;
    m_currentObjectPartData.nQuerySize = size;

    CWizApiTokenParam param(*this);

    param.AddString(_T("obj_guid"), strObjectGUID);
    param.AddString(_T("obj_type"), strObjectType);

    param.AddInt64(_T("start_pos"), pos);
    param.AddInt64(_T("part_size"), size);

    return callXmlRpc(&param, SyncMethod_DownloadObjectPart);
}

void CWizApiBase::onDownloadDataPart(const WIZOBJECTPARTDATA& data)
{
    if (data.strObjectGUID.isEmpty()) {
        WIZOBJECTDATA objectData;
        onDownloadObjectDataCompleted(objectData);
        return;
    }

    m_bDownloadingObject = false;
    m_nCurrentObjectAllSize = data.nObjectSize;
    m_currentObjectData.arrayData.append(data.arrayData);

    double fPercent = 0;
    if (data.nObjectSize > 0)
        fPercent = 100.0 * m_currentObjectData.arrayData.size() / double(data.nObjectSize);
    Q_EMIT progressChanged(int(fPercent));

    //QString info = m_currentObjectData.strDisplayName;
    //Q_EMIT processLog(tr("Downloaded : ") + QString::number((int)fPercent) + "%");

    if (data.bEOF) {
        onDownloadObjectDataCompleted(m_currentObjectData);
    } else {
        downloadNextPartData();
    }
}

// upload trunk data
bool CWizApiBase::uploadObjectData(const WIZOBJECTDATA& data)
{
    if (data.eObjectType == wizobjectDocument) {
        QString info = data.strDisplayName;
        Q_EMIT processLog(tr("uploading document data: ") + info);
    } else if (data.eObjectType == wizobjectDocumentAttachment) {
        QString info = data.strDisplayName;
        Q_EMIT processLog(tr("uploading attachment data: ") + info);
    } else {
        Q_ASSERT(false);
    }

    Q_ASSERT(!data.arrayData.isEmpty());

    m_currentObjectData = data;
    m_nCurrentObjectAllSize = data.arrayData.size();
    m_strCurrentObjectMD5 = ::WizMd5StringNoSpaceJava(m_currentObjectData.arrayData);

    return uploadNextPartData();
}

bool CWizApiBase::uploadNextPartData()
{
    if (m_currentObjectData.arrayData.isEmpty())
    {
        onUploadObjectDataCompleted(m_currentObjectData);
        return true;
    }

    int allSize = m_nCurrentObjectAllSize;
    int partSize = WIZAPI_TRUNK_SIZE;

    int partCount = allSize / partSize;
    if (allSize % partSize != 0)
    {
        partCount++;
    }

    int lastSize = m_currentObjectData.arrayData.size();

    int partIndex = (allSize - lastSize) / partSize;

    QByteArray arrayData;

    if (lastSize <= partSize)
    {
        arrayData = m_currentObjectData.arrayData;
        m_currentObjectData.arrayData.clear();
    }
    else
    {
        arrayData = QByteArray(m_currentObjectData.arrayData.constData(), partSize);
        m_currentObjectData.arrayData.remove(0, partSize);
    }

    return callUploadDataPart(m_currentObjectData.strObjectGUID,
                              WIZOBJECTDATA::ObjectTypeToTypeString(m_currentObjectData.eObjectType),
                              m_strCurrentObjectMD5,
                              allSize, partCount, partIndex, arrayData.size(), arrayData);
}

bool CWizApiBase::callUploadDataPart(const CString& strObjectGUID, const CString& strObjectType,
                                     const CString& strObjectMD5, int allSize,
                                     int partCount, int partIndex,
                                     int partSize, const QByteArray& arrayData)
{
    Q_UNUSED(allSize);

    Q_ASSERT(partSize == arrayData.size());

    CWizApiTokenParam param(*this);
    param.AddString(_T("obj_guid"), strObjectGUID);
    param.AddString(_T("obj_type"), strObjectType);
    param.AddString(_T("obj_md5"), strObjectMD5);
    param.AddInt(_T("part_count"), partCount);
    param.AddInt(_T("part_sn"), partIndex);
    param.AddInt64(_T("part_size"), partSize);
    param.AddString(_T("part_md5"), ::WizMd5StringNoSpaceJava(arrayData));
    param.AddBase64(_T("data"), arrayData);

    return callXmlRpc(&param, SyncMethod_UploadObjectPart);
}

void CWizApiBase::onUploadDataPart()
{
    if (m_currentObjectData.arrayData.isEmpty()) {
        onUploadObjectDataCompleted(m_currentObjectData);
    } else {
        uploadNextPartData();
    }
}


bool CWizApiBase::callGetList(const QString& strMethodName, qint64 nVersion)
{
    CWizApiTokenParam param(*this);
    param.AddInt("count", WIZAPI_PAGE_MAX);
    param.AddString("version", WizInt64ToStr(nVersion));

    return callXmlRpc(&param, strMethodName);
}

template <class TData>
inline bool CWizApiBase::callPostList(const QString& strMethodName,
                                      const CString& strArrayName,
                                      const std::deque<TData>& arrayData)
{
    if (arrayData.empty())
        return true;

    CWizApiTokenParam param(*this);

    param.AddArray<TData>(strArrayName, arrayData);

    return callXmlRpc(&param, strMethodName);
}



/* ------------------------------ CWizApi ------------------------------*/

CWizApi::CWizApi(CWizDatabase& db, const CString& strKbUrl /* = WIZ_API_URL */)
    : CWizApiBase(strKbUrl)
    , m_db(&db)
{
}

void CWizApi::onClientLogin(const WIZUSERINFO& userInfo)
{
    //m_db->setUserInfo(userInfo);
}

void CWizApi::onGetGroupList(const CWizGroupDataArray& arrayGroup)
{
    if (!arrayGroup.size())
        return;

    m_db->SetUserGroupInfo(arrayGroup);
}

//void CWizApi::onDeletedGetList(const std::deque<WIZDELETEDGUIDDATA>& arrayRet)
//{
//    m_db->UpdateDeletedGUIDs(arrayRet);
//}

//void CWizApi::onDeletedPostList(const std::deque<WIZDELETEDGUIDDATA>& arrayData)
//{
//    std::deque<WIZDELETEDGUIDDATA>::const_iterator it;
//    for (it = arrayData.begin(); it != arrayData.end(); it++) {
//        m_db->DeleteDeletedGUID(it->strGUID);
//    }
//}

//void CWizApi::onTagGetList(const std::deque<WIZTAGDATA>& arrayRet)
//{
//    m_db->UpdateTags(arrayRet);
//}

//void CWizApi::onTagPostList(const std::deque<WIZTAGDATA>& arrayData)
//{
//    std::deque<WIZTAGDATA>::const_iterator it;
//    for (it = arrayData.begin(); it != arrayData.end(); it++) {
//        m_db->ModifyObjectVersion(it->strGUID, WIZTAGDATA::ObjectName(), 0);
//    }
//}

//void CWizApi::onStyleGetList(const std::deque<WIZSTYLEDATA>& arrayRet)
//{
//    m_db->UpdateStyles(arrayRet);
//}

//void CWizApi::onStylePostList(const std::deque<WIZSTYLEDATA>& arrayData)
//{
//    std::deque<WIZSTYLEDATA>::const_iterator it;
//    for (it = arrayData.begin(); it != arrayData.end(); it++) {
//        m_db->ModifyObjectVersion(it->strGUID, WIZSTYLEDATA::ObjectName(), 0);
//    }
//}

//void CWizApi::onDocumentGetData(const WIZDOCUMENTDATAEX& data)
//{
//    m_db->UpdateDocument(data);
//}

//void CWizApi::onAttachmentGetList(const std::deque<WIZDOCUMENTATTACHMENTDATAEX>& arrayRet)
//{
//    m_db->UpdateAttachments(arrayRet);
//}

void CWizApi::onDownloadObjectDataCompleted(const WIZOBJECTDATA& data)
{
    m_db->UpdateSyncObjectLocalData(data);
}

bool CWizApi::uploadDocument(const WIZDOCUMENTDATAEX& data)
{
    Q_ASSERT(data.nObjectPart != 0);

    int nParts = data.nObjectPart;
    bool bInfo = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_INFO) ? true : false;
    bool bParam = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_PARAM) ? true : false;
    bool bData = (nParts & WIZKM_XMKRPC_DOCUMENT_PART_DATA) ? true : false;

    m_currentDocument = data;

    m_db->GetDocumentTags(m_currentDocument.strGUID, m_currentDocument.arrayTagGUID);
    m_db->GetDocumentParams(m_currentDocument.strGUID, m_currentDocument.arrayParam);

    if (bData) {
        if (!m_db->LoadDocumentData(m_currentDocument.strGUID, m_currentDocument.arrayData)) {
            QString info = data.strTitle;
            Q_EMIT processErrorLog(tr("could not load document data: ") + info);

            //skip
            onUploadDocument(data);
            return false;
        }

        WIZOBJECTDATA obj;
        obj.strObjectGUID = m_currentDocument.strGUID;
        obj.strDisplayName = m_currentDocument.strTitle;
        obj.eObjectType = wizobjectDocument;
        obj.arrayData = m_currentDocument.arrayData;

        return uploadObjectData(obj);
    } else {
        return callDocumentPostData(m_currentDocument);
    }
}

void CWizApi::onDocumentPostData(const WIZDOCUMENTDATAEX& data)
{
    //Q_ASSERT(data.strGUID == m_currentDocument.strGUID);
    onUploadDocument(m_currentDocument);
}

bool CWizApi::uploadAttachment(const WIZDOCUMENTATTACHMENTDATAEX& data)
{
    Q_ASSERT(data.nObjectPart != 0);

    int nParts = data.nObjectPart;
    bool bData = (nParts & WIZKM_XMKRPC_ATTACHMENT_PART_DATA) ? true : false;

    m_currentAttachment = data;
    if (bData) {
        if (!m_db->LoadCompressedAttachmentData(m_currentAttachment.strGUID, m_currentAttachment.arrayData)) {
            QString info = data.strName;
            Q_EMIT processErrorLog(tr("Could not load attachment data: ") + info);

            //skip
            onUploadAttachment(data);

            //re-upload attachment at next time
            m_db->ModifyObjectVersion(data.strGUID, WIZDOCUMENTATTACHMENTDATAEX::ObjectName(), -1);
            return false;
        }

        WIZOBJECTDATA obj;
        obj.strObjectGUID = m_currentAttachment.strGUID;
        obj.strDisplayName = m_currentAttachment.strName;
        obj.eObjectType = wizobjectDocumentAttachment;
        obj.arrayData = m_currentAttachment.arrayData;

        return uploadObjectData(obj);
    } else {
        return callAttachmentPostData(data);
    }
}

void CWizApi::onAttachmentPostData(const WIZDOCUMENTATTACHMENTDATAEX& data)
{
    Q_ASSERT(data.strGUID == m_currentAttachment.strGUID);
    onUploadAttachment(m_currentAttachment);
}

void CWizApi::onUploadObjectDataCompleted(const WIZOBJECTDATA& data)
{
    // upload data info after complete upload trunk data
    if (data.eObjectType == wizobjectDocument) {
        Q_ASSERT(data.strObjectGUID == m_currentDocument.strGUID);
        Q_ASSERT(m_currentDocument.nObjectPart & WIZKM_XMLRPC_OBJECT_PART_DATA);

        m_db->SetObjectDataDownloaded(m_currentDocument.strGUID, "document", true);
        callDocumentPostData(m_currentDocument);

    } else if (data.eObjectType == wizobjectDocumentAttachment) {
        Q_ASSERT(data.strObjectGUID == m_currentAttachment.strGUID);
        Q_ASSERT(m_currentAttachment.nObjectPart & WIZKM_XMLRPC_OBJECT_PART_DATA);

        m_db->SetObjectDataDownloaded(m_currentAttachment.strGUID, "attachment", true);
        callAttachmentPostData(m_currentAttachment);

    } else {
        Q_ASSERT(0);
    }
}



CWizApiParamBase::CWizApiParamBase()
{
    AddString("api_version", WIZ_API_VERSION);
    AddString("client_type", WIZ_CLIENT_TYPE);
    AddString("client_version", WIZ_CLIENT_VERSION);
}

CWizApiTokenParam::CWizApiTokenParam(CWizApiBase& api)
{
    AddString("token", WizGlobal()->token());
    AddString("kb_guid", api.kbGUID());
}
