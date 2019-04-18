

#include "custom-router-2.hpp"
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

NS_LOG_COMPONENT_DEFINE("CustomRouter2");

namespace ns3 {
namespace ndn {
  
  NS_OBJECT_ENSURE_REGISTERED(CustomRouter2);

  //サーバのID登録
  TypeId
  CustomRouter2::GetTypeId()
  {
    static TypeId tid = 
      TypeId("CustomRouter2")
      .SetParent<ndn::App>()
      .AddConstructor<CustomRouter2>();
    return tid;
  }
  CustomRouter2::CustomRouter2()
  {
    NS_LOG_FUNCTION_NOARGS();
  }
  // Processing upon start of the application
  void
  CustomRouter2::StartApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    ndn::App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(),"/prefix", m_face,0);
    ndn::FibHelper::AddRoute(GetNode(),"/verification", m_face,0);
  }

  void
  CustomRouter2::StopApplication()
  {
    NS_LOG_FUNCTION_NOARGS();
    ndn::App::StopApplication();
  }

  //void
  //CustomRouter2::SendInterest()
  //{
  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////

  // Create and configure ndn::Interest
  //  auto interest = std::make_shared<ndn::Interest>("/prefix/sub");
  //  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  //  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  //  interest->setInterestLifetime(ndn::time::seconds(1));

  //  NS_LOG_DEBUG("Sending Interest packet for " << *interest);

  // Call trace (for logging purposes)
  //  m_transmittedInterests(interest, this, m_face);

  //  m_appLink->onReceiveInterest(*interest);
  //}

  //要求パケットが届いたのちに対応するデータを返す
  void
  CustomRouter2::OnInterest(std::shared_ptr<const ndn::Interest> interest)
  {

    ndn::App::OnInterest(interest);
    if(!m_active)
      return;
    NS_LOG_DEBUG("Received Interest packet for " << interest->getName());

    // auto data = std::make_shared<ndn::Data>(interest->getName());//要求パケットの名前と同じデータを作成
    // data->setName(interest->getName()); 
    // data->setFreshnessPeriod(ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    // data->setContent(std::make_shared< ::ndn::Buffer>(m_virtualPayloadSize));//コンテンツを追加
    //Signature signature;
    //SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
    //ndn::StackHelper::getKeyChain().sign(*data);//データに認証を付加
//     if (m_keyLocator.size() > 0) {
//       signatureInfo.setKeyLocator(m_keyLocator);
 //    }
   //  signature.setInfo(signatureInfo);
     //signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

     //data->setSignature(signature);
     //NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
    //data->wireEncode();
    //privatekey = ndn::security::transform::PrivateKey()::generatePrivateKey(ndn::KeyParams::KeyParams(RSA,SHA256));
    //publickey = ndn::security::transform::PublicKey()::encrypt(*data)	
    m_transmittedInterests(interest, this, m_face);//データを送ったことを示すトレースを返す
    m_appLink->onReceiveInterest(*interest);//データを配送
    // NS_LOG_DEBUG("Sending Data packet for " << data->getName());//データを送ることをデバックで報告
  }
  // void
  // CustomRouter2::OnData(std::shared_ptr<const ndn::Data> data)
  // {
  // NS_LOG_DEBUG("Receiving Data packet for " << data->getName());

  // //  std::cout << "DATA received for name " << data->getName() << std::endl;
  // }
}//ndnの名前空間の終わり
}//ns3の名前空間の終わり
