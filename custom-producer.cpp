#include "custom-producer.hpp"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/random-variable-stream.h"
#include "ns3/ndnSIM/ndn-cxx/security/security-common.hpp"

#define IDENTITY_NAME "NDNSERVER"

NS_LOG_COMPONENT_DEFINE("CustomProducer");

namespace ns3 {
namespace ndn {
  
  NS_OBJECT_ENSURE_REGISTERED(CustomProducer);

  //サーバのID登録
TypeId
  CustomProducer::GetTypeId()
  {
    static TypeId tid = 
      TypeId("CustomProducer")
      .SetParent<ndn::App>()
      //親はAppクラス
      .AddConstructor<CustomProducer>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", 
          StringValue("/"),
          MakeNameAccessor(&CustomProducer::m_prefix), 
          MakeNameChecker())
      .AddAttribute("Postfix",
          "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
          StringValue("/"), MakeNameAccessor(&CustomProducer::m_postfix), 
          MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", 
          UintegerValue(1024),
          MakeUintegerAccessor(&CustomProducer::m_virtualPayloadSize),
          MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
          TimeValue(Seconds(0)), MakeTimeAccessor(&CustomProducer::m_freshness),
          MakeTimeChecker())
      .AddAttribute(
          "Signature",
          "Fake signature, 0 valid signature (default), other values application-specific",
          UintegerValue(0), MakeUintegerAccessor(&CustomProducer::m_signature),
          MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&CustomProducer::m_keyLocator), 
  MakeNameChecker());
    return tid;
  }
  
  CustomProducer::CustomProducer()
    :m_identityName(IDENTITY_NAME)
  {
    NS_LOG_FUNCTION_NOARGS();
  }
  
  //アプリケーションのスタート
  void
  CustomProducer::StartApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    ndn::App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(),"/prefix", m_face,0);
    ndn::FibHelper::AddRoute(GetNode(),"/verification", m_face,0);
  }

  // Processing when application is stopped
  void
  CustomProducer::StopApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    // cleanup ndn::App
    ndn::App::StopApplication();
  }
  
  //要求パケットが届いたのちに対応するデータを返す
  void
  CustomProducer::OnInterest(std::shared_ptr<const ndn::Interest> interest)
  {

    ndn::App::OnInterest(interest);
    if(!m_active)
      return;
    NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

    // Note that Interests send out by the app will not be sent back to the app !

    auto data = std::make_shared<ndn::Data>(interest->getName());
    //要求パケットの名前と同じデータを作成
    data->setName(interest->getName()); 
    data->setFreshnessPeriod(ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    data->setContent(std::make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
    
    //コンテンツを追加
    m_keyChain.createIdentity(m_identityName);
    m_keyChain.sign(*data, signingByIdentity(m_identityName));
    //データに認証を付加

    
    m_transmittedDatas(data, this, m_face);
    //データを送ったことを示すトレースを返す
    m_appLink->onReceiveData(*data);
    //データを配送
    NS_LOG_DEBUG("Sending Data packet for " << data->getName());
    //データを送ることをデバックで報告
  }

}//ndnの名前空間の終わり
}//ns3の名前空間の終わり
