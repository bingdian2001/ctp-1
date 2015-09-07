#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <time.h>
#include "zmsg.hpp"
#include <vector>
#include <string>
#include <map>
#include "spdlog/spdlog.h"
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

size_t spd_size = 1048576;
//spdlog->set_async_mode(spd_size);
auto console = spdlog::stdout_logger_mt("console");
auto logger = spdlog::rotating_logger_mt("log", "log", spd_size * 5, 10);


std::string CoreServer = "tcp://192.168.1.234:5555";
using std::string;
class Carbon;

class MD : public CThostFtdcMdSpi
{
private:
	Carbon *pC;
public:
	MD(Carbon* _pC):pC(_pC) {}

	///错误应答
	void OnRspError(CThostFtdcRspInfoField *pRspInfo,int nRequestID, bool bIsLast);

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	void OnHeartBeatWarning(int nTimeLapse);

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	void OnFrontConnected();

	///登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅行情应答
	void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///取消订阅行情应答
	void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///深度行情通知
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
};

class TD : public CThostFtdcTraderSpi
{
private:
	Carbon *pC;
public:
	TD(Carbon* _pC) :pC(_pC) {}
	
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。

	void OnFrontConnected();

	///登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果请求响应
	void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约响应
	void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	void OnHeartBeatWarning(int nTimeLapse);

	///报单通知
	void OnRtnOrder(CThostFtdcOrderField *pOrder);

	///成交通知
	void OnRtnTrade(CThostFtdcTradeField *pTrade);

	///请求查询报单响应
	void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询成交响应
	void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓明细响应
	void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

};

///===================================================================================================================
class Carbon 
{
private:
	void			*m_socket;	//	for zmq
	void			*m_context;	//	for zmq
	MD			*pmds;	//	Md Spi
	TD			*ptds;	//	Td Spi
	CThostFtdcMdApi		*pmda;	//	Md Api
	CThostFtdcTraderApi	*ptda;	//	Td Api
	std::string		md_front,td_front;
	std::string		broker_id;
	std::string		account_num;
	std::string		account_pwd;
	std::string		this_symbol;
	int			request_id;
	int			order_ref;
	int			front_id;
	int			session_id;
	time_t			timer;
	bool			is_initing;

public:
	Carbon(std::string server_) {
		m_context	= zmq_ctx_new();
		m_socket	= zmq_socket(m_context, ZMQ_REQ);
		zmq_connect(m_socket, server_.c_str());
		request_id = 0;
		order_ref = 0;
		is_initing = true;
	}

	~Carbon() {
		if (m_socket != NULL) {
			zmq_close(m_socket);
			m_socket = NULL;
		}
	}

	void join() {
		pmda->Join();
		ptda->Join();
	}

	int init() {
		md_front = msg("md_front");
		info("===获取行情地址[MD_Front]===");
		info(md_front);
		td_front = msg("td_front");
		info("===获取交易地址[TD_Front]===");// << std::endl;
		info(td_front);// << std::endl;
		broker_id = msg("broker_id");
		info("===获取座席编号[BrokerID]===");// << std::endl;
		info(broker_id);// << std::endl;
		account_num = msg("account_num");
		info("===获取账号[InvestorID]===");// << std::endl;
		info(account_num);// << std::endl;
		account_pwd = msg("account_pwd");
		info("===获取密码[Password]===");// << std::endl;
		info("ok!");// << std::endl;
		this_symbol = msg("this_symbol");
		info("===设定合约===");// << std::endl;
		info(this_symbol);// << std::endl;

		char *md_buf = new char[strlen(md_front.data()) + 1];
		strcpy(md_buf, md_front.c_str());

		char *td_buf = new char[strlen(td_front.data()) + 1];
		strcpy(td_buf, td_front.c_str());

		timer = time(NULL);

		ptda = CThostFtdcTraderApi::CreateFtdcTraderApi(".\\tdflow\\");
		ptds = new TD(this);
		ptda->RegisterSpi((CThostFtdcTraderSpi*)ptds);
		ptda->SubscribePublicTopic(THOST_TERT_QUICK);
		ptda->SubscribePrivateTopic(THOST_TERT_QUICK);
		ptda->RegisterFront(td_buf);
		
		pmda = CThostFtdcMdApi::CreateFtdcMdApi(".\\mdflow\\");
		pmds = new MD(this);
		pmda->RegisterSpi(pmds);
		pmda->RegisterFront(md_buf);

		//  启动初始化
		info("TD_init");
		ptda->Init();	//	触发TD::OnFrontConnected，启动初始化链条
		//初始化链条完毕后pmda->init()；然后一起join
		info("TD 开始初始化...");// << std::endl;
		return 0;
	}

	int get_request_id() {
		return ++request_id;
	}

	int get_order_ref() {
		return ++order_ref;
	}

	std::string log(std::string s_) {
		msg("log_" + s_);
		info(s_);
		return "logged:"+s_;
	}
	void info(std::string s_) {
		console->info(s_);
		logger->info(s_);
	}
	void debug(std::string s_) {
		log("debug_" + s_);
	}
	std::string error(int err_) {
		std::stringstream ss_;
		std::string sto_;
		ss_ << err_;
		ss_ >> sto_;
		return msg("error_" + sto_);
	}

	std::string msg(std::string s_) {
		zmsg msg_(s_.data());
		msg_.send((zmq::socket_t &)m_socket);
		msg_.recv((zmq::socket_t &)m_socket);
		return msg_.body();
	}

	bool isErrorRspInfo(CThostFtdcRspInfoField *pRspInfo){
		bool ret_ = ((pRspInfo) && (pRspInfo->ErrorID != 0));
		if (ret_){
			info("!!! CTP报错 [ " + error(pRspInfo->ErrorID) + " ] !!!");// << std::endl;
		}
		return ret_;
	}
	void tdOnFrontConnected(){
		info(__FUNCTION__);
		tdReqUserLogin();
	}
	void tdReqUserLogin() {
		info(__FUNCTION__);
		CThostFtdcReqUserLoginField req_;
		memset(&req_, 0, sizeof(req_));
		strcpy(req_.BrokerID, broker_id.data());
		strcpy(req_.UserID, account_num.data());
		strcpy(req_.Password, account_pwd.data());
		int ret_ = ptda->ReqUserLogin(&req_, get_request_id());
		info("TD 发送登录请求: ");//  
		info((ret_ == 0) ? "成功" : "失败");
	}

	void tdOnRspUserLogin(CThostFtdcRspUserLoginField *p) {
		info(__FUNCTION__);
		front_id = p->FrontID;
		session_id = p->SessionID;
		order_ref = atoi(p->MaxOrderRef);
		info("TD 登录成功...");// >> > FrontID: " << front_id << " SessionID : " << session_id << " OrderRef : " << order_ref << std::endl;
		if (msg("confirmed_today") == "0") {
			tdReqQrySettlementInfoConfirm();
		}
		else {
			info("跳过结算单确认...");// << "#===" << std::endl;
			tdReqQryTradingAccount();
		}
	}
	void tdReqQrySettlementInfoConfirm() {
		bool do_it = false;
		if (is_initing) {
			Sleep(3000);
			do_it = true;
		}
		else if(time(NULL)-timer>=1) {
			timer = time(NULL);
			do_it = true;
		}
		if (do_it) {
			info(__FUNCTION__);
			CThostFtdcQrySettlementInfoConfirmField req_;
			memset(&req_, 0, sizeof(req_));
			strcpy(req_.BrokerID, broker_id.data());
			strcpy(req_.InvestorID, account_num.data());
			int ret_ = ptda->ReqQrySettlementInfoConfirm(&req_, get_request_id());
			info("TD 发送结算单确认请求: ");//  
			info((ret_ == 0) ? "成功" : "失败");
		}
	}

	void tdOnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *p) {
		info(__FUNCTION__);
		if (p) {
			msg("confirm_today");
			info("TD 结算单确认成功...");
		}
		else {
			info("TD 结算单已确认...");// << std::endl;
		}
		tdReqQryTradingAccount();
	}

	void tdReqQryTradingAccount() {
		bool do_it = false;
		if (is_initing) {
			Sleep(3000);
			do_it = true;
		}
		else if (time(NULL) - timer >= 1) {
			timer = time(NULL);
			do_it = true;
		}
		if (do_it) {
			info(__FUNCTION__);
			CThostFtdcQryTradingAccountField req_;
			memset(&req_, 0, sizeof(req_));
			strcpy(req_.BrokerID, broker_id.data());
			strcpy(req_.InvestorID, account_num.data());
			int ret_ = ptda->ReqQryTradingAccount(&req_, get_request_id());
			info("TD 发送帐户查询请求: ");
			info((ret_ == 0) ? "成功" : "失败");
		}
	}

	void tdOnRspQryTradingAccount(CThostFtdcTradingAccountField *p) {
		info(__FUNCTION__);
		info("TD 帐户查询成功:");// << std::endl;
		info("可用资金: ");//
		std::cout << p->Available << std::endl;
		tdReqQryInvestorPosition();
	}

	void tdReqQryInvestorPosition() {
		bool do_it = false;
		if (is_initing) {
			Sleep(3000);
			do_it = true;
		}
		else if (time(NULL) - timer >= 1) {
			timer = time(NULL);
			do_it = true;
		}
		if (do_it) {
			info(__FUNCTION__);
			CThostFtdcQryInvestorPositionField req_;
			memset(&req_, 0, sizeof(req_));
			strcpy(req_.BrokerID, broker_id.data());
			strcpy(req_.InvestorID, account_num.data());
			strcpy(req_.InstrumentID, this_symbol.data());
			int ret_ = ptda->ReqQryInvestorPosition(&req_, get_request_id());
			info("TD 发送持仓查询请求: ");
			info((ret_ == 0) ? "成功" : "失败");
		}
	}

	void tdOnRspQryInvestorPosition(CThostFtdcInvestorPositionField *p) {
		info(__FUNCTION__);
		info("TD 持仓查询成功:");// << std::endl;
		if (p) {
			std::cout << "今日持仓: " << p->TodayPosition << std::endl;
		}
		else {
			std::cout << "今日空仓... " << std::endl;
		}
		info("TD 初始化结束...");// << std::endl;
		info("MD 开始初始化...");// << std::endl;
		info("MD_init");
		pmda->Init();
	}

	void tdOnRspQryOrder(CThostFtdcOrderField *p){
		info(__FUNCTION__);

	}
	void tdOnRspOrderInsert(CThostFtdcInputOrderField *p){
		info(__FUNCTION__);

	}
	void tdOnRspOrderAction(CThostFtdcInputOrderActionField *p){
		info(__FUNCTION__);

	}
	void tdOnRtnOrder(CThostFtdcOrderField *p){
		info(__FUNCTION__);

	}
	void tdOnRtnTrade(CThostFtdcTradeField *p){
		info(__FUNCTION__);

	}
	void tdOnRspQryTrade(CThostFtdcTradeField *p){
		info(__FUNCTION__);

	}
	void tdOnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *p){
		info(__FUNCTION__);

	}
	
	void mdOnFrontConnected(){
		info(__FUNCTION__);
		mdReqUserLogin();
	}
	void mdReqUserLogin() {
		info(__FUNCTION__);
		CThostFtdcReqUserLoginField req_;
		memset(&req_, 0, sizeof(req_));
		strcpy(req_.BrokerID, broker_id.data());
		strcpy(req_.UserID, account_num.data());
		strcpy(req_.Password, account_pwd.data());
		int ret_ = pmda->ReqUserLogin(&req_, get_request_id());
		info("MD 发送登录请求: ");
		info((ret_ == 0) ? "成功" : "失败");
	}

	void mdOnRspUserLogin(CThostFtdcRspUserLoginField *p) {
		info(__FUNCTION__);
		info("MD 登录成功...");// << std::endl;
		mdSubscribeMarketData();
	}

	void mdSubscribeMarketData() {
		info(__FUNCTION__);
		int len_ = 1;
		char** p_inst_id = new char*[len_];
		char *out_ = new char[strlen(this_symbol.data()) + 1];
		strcpy(out_,this_symbol.data());
		for (int i = 0; i < len_; i++)	p_inst_id[i] = out_;
		int ret_ = pmda->SubscribeMarketData(p_inst_id, len_);
		info("MD 发送订阅行情请求: ");
		info((ret_ == 0) ? "成功" : "失败");
	}

	void mdOnRspSubMarketData(CThostFtdcSpecificInstrumentField *p) {
		info(__FUNCTION__);
		info("MD 订阅行情成功...");// << std::endl;
		is_initing = false;
	}

	void mdOnRtnDepthMarketData(CThostFtdcDepthMarketDataField *p) {
		info(__FUNCTION__);
		std::cout << p->UpdateTime <<" : "<< p->LastPrice << std::endl;
	}
};
//===========================================================================================================================

Carbon*	pC;

//===========================================================================================================================
// for MD Spi
void MD::OnRspError(CThostFtdcRspInfoField *pRspInfo,int nRequestID, bool bIsLast) {
	pC->isErrorRspInfo(pRspInfo);
};

void MD::OnFrontDisconnected(int nReason) {
	pC->info("!!! MD 客户端连接中断...");
};

void MD::OnHeartBeatWarning(int nTimeLapse) {
	pC->info("!!! MD 心跳超时警告...");
};

void MD::OnFrontConnected() {
	pC->mdOnFrontConnected();
};

void MD::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pRspUserLogin) {
		pC->mdOnRspUserLogin(pRspUserLogin);
	}
};

void MD::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo)) {
		pC->mdOnRspSubMarketData(pSpecificInstrument);
	}
};

void MD::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
};

void MD::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
	pC->mdOnRtnDepthMarketData(pDepthMarketData);
};

//===========================================================================================================================
// for TD Spi
//	初始化链条
void TD::OnFrontConnected() {
	pC->tdOnFrontConnected();
};
//	初始化链条
void TD::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pRspUserLogin){
		pC->tdOnRspUserLogin(pRspUserLogin);
	}
};

void TD::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
};

//	初始化链条
void TD::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo)) {
		pC->tdOnRspSettlementInfoConfirm(pSettlementInfoConfirm);
	}
};

void TD::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
};
//	初始化链条
void TD::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pTradingAccount) {
		pC->tdOnRspQryTradingAccount(pTradingAccount);
	}
};
//	初始化链条
void TD::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo)) {
		pC->tdOnRspQryInvestorPosition(pInvestorPosition);
	}
};

void TD::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	pC->isErrorRspInfo(pRspInfo);
};

void TD::OnFrontDisconnected(int nReason) {
	pC->info("!!! TD 客户端连接中断...");
};

void TD::OnHeartBeatWarning(int nTimeLapse) {
	pC->info("!!! TD 心跳超时警告...");
};
///	报单查询响应
void TD::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pOrder) {
		pC->tdOnRspQryOrder(pOrder);
	}
};
///	报单响应
void TD::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pInputOrder) {
		pC->tdOnRspOrderInsert(pInputOrder);
	}
};
///	撤单响应
void TD::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pInputOrderAction) {
		pC->tdOnRspOrderAction(pInputOrderAction);
	}
};
///	成交查询响应
void TD::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pTrade) {
		pC->tdOnRspQryTrade(pTrade);
	}
};
///	持仓详细查询响应
void TD::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if (!pC->isErrorRspInfo(pRspInfo) && pInvestorPositionDetail) {
		pC->tdOnRspQryInvestorPositionDetail(pInvestorPositionDetail);
	}
};
///	报单回报
void TD::OnRtnOrder(CThostFtdcOrderField *pOrder) {
	if (pOrder) {
		pC->tdOnRtnOrder(pOrder);
	}
};
///	成交回报
void TD::OnRtnTrade(CThostFtdcTradeField *pTrade) {
	if (pTrade) {
		pC->tdOnRtnTrade(pTrade);
	}
};


//===========================================================================================================================

int main(int argc,const char* argv[])
{
	SetConsoleTitle(_T("CTP交易终端 [qq:129769]"));
	if ( argc > 1) {
		std::cout << ">>>连接自定义服务器" << std::endl;
		pC = new Carbon(argv[1]);
	}
	else{
		std::cout << ">>>连接默认服务器" << std::endl;
		pC = new Carbon(CoreServer);
	}
	pC->info("===================================");
	pC->init();
	pC->join();
    return 0;
}

