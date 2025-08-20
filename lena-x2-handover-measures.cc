/*
 * Copyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/three-gpp-propagation-loss-model.h"
// #include "ns3/config-store.h"
#include <math.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaX2HandoverMeasures");

void
NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << context << " UE IMSI " << imsi << ": connected to CellId " << cellid
              << " with RNTI " << rnti << std::endl;
}

void
NotifyHandoverStartUe(std::string context,
                      uint64_t imsi,
                      uint16_t cellid,
                      uint16_t rnti,
                      uint16_t targetCellId)
{
    std::cout << context << " UE IMSI " << imsi << ": previously connected to CellId " << cellid
              << " with RNTI " << rnti << ", doing handover to CellId " << targetCellId
              << std::endl;
}

void
NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << context << " UE IMSI " << imsi << ": successful handover to CellId " << cellid
              << " with RNTI " << rnti << std::endl;
}

void
NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << context << " eNB CellId " << cellid << ": successful connection of UE with IMSI "
              << imsi << " RNTI " << rnti << std::endl;
}

void
NotifyHandoverStartEnb(std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
    std::cout << context << " eNB CellId " << cellid << ": start handover of UE with IMSI " << imsi
              << " RNTI " << rnti << " to CellId " << targetCellId << std::endl;
}

void
NotifyHandoverEndOkEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << context << " eNB CellId " << cellid << ": completed handover of UE with IMSI "
              << imsi << " RNTI " << rnti << std::endl;
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
  {
    double localThrou=0;
    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
       if (fiveTuple.sourceAddress == Ipv4Address("1.0.0.2") /*|| fiveTuple.sourceAddress == Ipv4Address("7.0.0.2") || fiveTuple.sourceAddress == Ipv4Address("7.0.0.3")*/)
      {
      
      std::cout<<"Flow ID     : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
      std::cout<<"Tx Packets : " << stats->second.txPackets<<std::endl;
      std::cout<<"Rx Packets : " << stats->second.rxPackets<<std::endl;
      std::cout<<"Duration    : "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
      std::cout<<"Last Received Packet  : "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
      std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024  << " Kbps"<<std::endl;
      std::cout<< "Mean{Delay}: " << (stats->second.delaySum.GetSeconds()/stats->second.rxPackets) << "\n";
      std::cout<< "Mean{Jitter}: " << (stats->second.jitterSum.GetSeconds()/(stats->second.rxPackets)) << "\n";
      std::cout<< "Total{Delay}: " << (stats->second.delaySum.GetSeconds()) << "\n";
      std::cout<< "Total{Jitter}: " << (stats->second.jitterSum.GetSeconds()) << "\n";
      std::cout<< "Lost Packets: " << (stats->second.lostPackets) << "\n";
      std::cout<< "Dropped Packets: " << (stats->second.packetsDropped.size()) << "\n";
      localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024);
      // updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
      std::cout<<"---------------------------------------------------------------------------"<<std::endl;
      }
    }
      Simulator::Schedule(Seconds(0.2  ),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
  flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      }

  }


/**
 * Sample simulation script for an automatic X2-based handover based on the RSRQ measures.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB.
 * The UE moves between both eNBs, it reports measures to the serving eNB and
 * the 'source' (serving) eNB triggers the handover of the UE towards
 * the 'target' eNB when it considers it is a better eNB.
 */
int
main(int argc, char* argv[])
{
    LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_ALL);

    // LogComponentEnable ("LteHelper", logLevel);
    // LogComponentEnable ("EpcHelper", logLevel);
    // LogComponentEnable ("EpcEnbApplication", logLevel);
    // LogComponentEnable ("EpcMmeApplication", logLevel);
    // LogComponentEnable ("EpcPgwApplication", logLevel);
    // LogComponentEnable ("EpcSgwApplication", logLevel);
    // LogComponentEnable ("EpcX2", logLevel);

//    LogComponentEnable ("LteEnbRrc", logLevel);
    // LogComponentEnable ("LteEnbNetDevice", logLevel);
    // LogComponentEnable ("LteUeRrc", logLevel);
    // LogComponentEnable ("LteUeNetDevice", logLevel);
    LogComponentEnable ("A2A4RsrqHandoverAlgorithm", logLevel);
    LogComponentEnable ("A3RsrpHandoverAlgorithm", logLevel);

    uint16_t numberOfUes = 1;
    uint16_t numberOfEnbs = 3;
    uint16_t numBearersPerUe = 1;
    double distance = 200.0;                                        // m
    double speed = 16.4;                                              // m/s
    double simTime = (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
    double enbTxPowerDbm = 46.0;

    // change some default attributes so that they are reasonable for
    // this scenario, but do this before processing command line
    // arguments, so that the user is allowed to override these settings
    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(2000)));
//    Config::SetDefault ("ns3::UdpClient::PacketSize", UintegerValue (1024));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
    Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(2));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);
    cmd.AddValue("speed", "Speed of the UE (default = 20 m/s)", speed);
    cmd.AddValue("enbTxPowerDbm", "TX power [dBm] used by HeNBs (default = 46.0)", enbTxPowerDbm);

    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");


    // Urban Macrocell channel

    // Pick the scenario (Urban Macro = Uma)

    lteHelper->SetPathlossModelType(ThreeGppUmaPropagationLossModel::GetTypeId());
    
    // Setup fading model for vehicle moving at 60km/h
    lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
	lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad"));
	lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10)));
	lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
	lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
	lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));


//    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
//    lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold", UintegerValue(30));
//    lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset", UintegerValue(1));

      lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
      lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                                DoubleValue (3.0));
      lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                                TimeValue (MilliSeconds (256)));

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Routing of the Internet Host (towards the LTE network)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    // interface 0 is localhost, 1 is the p2p device
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    /*
     * Network topology:
     *
     *      |     + --------------------------------------------------------->
     *      |     UE
     *      |
     *      |               d                   d                   d
     *    y |     |-------------------x-------------------x-------------------
     *      |     |                 eNodeB              eNodeB
     *      |   d |
     *      |     |
     *      |     |                                             d = distance
     *            o (0, 0, 0)                                   y = yForUe
     */

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    // Install Mobility Model in eNB
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();

    Vector enbPosition1(200, 200, 25);
    enbPositionAlloc->Add(enbPosition1);
    Vector enbPosition2(500, 200, 25);
    enbPositionAlloc->Add(enbPosition2);
    Vector enbPosition3(800, 200, 25);
    enbPositionAlloc->Add(enbPosition3);
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    // Install Mobility Model in UE
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    
//	ueMobility.SetPositionAllocator("ns3::GridPositionAllocator",
//		                            "MinX", DoubleValue(450.0),
//		                            "MinY", DoubleValue(344.0),
//		                            "DeltaX", DoubleValue(5.0),
//		                            "DeltaY", DoubleValue(5.0),
//		                            "GridWidth", UintegerValue(1),
//		                            "LayoutType", StringValue("RowFirst"));

//	ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
//		                        "Bounds", RectangleValue(Rectangle(200, 1500, 200, 1500)), // movement area
//		                        "Distance", DoubleValue(500.0),   // travel this distance before changing direction
//		                        "Speed", StringValue("ns3::UniformRandomVariable[Min=10.0|Max=16.4]"), // random speed range
//		                        "Direction", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283185]")); // 0–2π radians
	ueMobility.Install(ueNodes);
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(100, 300, 1.5));
    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(speed, 0, 0));
    // Install LTE Devices in eNB and UEs
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(enbTxPowerDbm));
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // Attach all UEs to the first eNodeB
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));
    }

    NS_LOG_LOGIC("setting up applications");

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ++dlPort;
            ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            NS_LOG_LOGIC("installing UDP DL app for UE " << u);
            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
            clientApps.Add(dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ue));

            NS_LOG_LOGIC("installing UDP UL app for UE " << u);
            UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
            clientApps.Add(ulClientHelper.Install(ue));
            PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

            Ptr<EpcTft> tft = Create<EpcTft>();
            EpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            tft->Add(dlpf);
            EpcTft::PacketFilter ulpf;
            ulpf.remotePortStart = ulPort;
            ulpf.remotePortEnd = ulPort;
            tft->Add(ulpf);
            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
        }
    }

    // Add X2 interface
    lteHelper->AddX2Interface(enbNodes);

    // X2-based Handover
    // lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (0),
    // enbLteDevs.Get (1));

    // Uncomment to enable PCAP tracing
    p2ph.EnablePcapAll("lena-x2-handover-measures");

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    lteHelper->EnablePdcpTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.02)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.02)));

    // connect custom trace sinks for RRC connection establishment and handover notification
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkUe));

    Simulator::Stop(Seconds(simTime));
    
    std::cout << "Total nodes: " << NodeList::GetNNodes() << std::endl;
    for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i)
    {
       Ptr<Node> node = NodeList::GetNode(i);
       std::cout << "Node " << i << " ID=" << node->GetId() << std::endl;
    }

  AnimationInterface anim ("lte2.xml");
  //anim.EnablePacketMetadata ();
  //Ptr<Node> mme    = epcHelper->GetMmeNode();
  anim.SetMaxPktsPerTraceFile (100000000000);
  anim.SetMobilityPollInterval(Seconds(2));

// Loop through all nodes and assign names
for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i)
{
    Ptr<Node> node = NodeList::GetNode(i);
    std::string desc;

    if (i == 0) desc = "RemoteHost";
    else if (i == 1) desc = "PGW";
    else if (i == 2) desc = "MME";
    else if (i == 3) desc = "SGW";
    else if (i == 4) desc = "eNB-1";
    else if (i == 5) desc = "eNB-2";
    else if (i == 6) desc = "eNB-3";
    else if (i == 7) desc = "UE-1";

    anim.UpdateNodeDescription(node, desc);

}

uint32_t remoteImg = anim.AddResource("/home/vboxuser/ns-3.45/scratch/remote.png");
//uint32_t pgwImg    = anim.AddResource("/home/vboxuser/ns-3.45/scratch/pgw.png");
//uint32_t mmeImg    = anim.AddResource("/home/vboxuser/ns-3.45/scratch/mme.png");
//uint32_t sgwImg    = anim.AddResource("/home/vboxuser/ns-3.45/scratch/sgw.png");
uint32_t enbImg    = anim.AddResource("/home/vboxuser/ns-3.45/scratch/enb.png");
uint32_t ueImg     = anim.AddResource("/home/vboxuser/ns-3.45/scratch/ue.png");
// Replace node shapes with images
anim.UpdateNodeImage(0, remoteImg);  // RemoteHost
//anim.UpdateNodeImage(NodeList::GetNode(1), "pgw.png");     // PGW
//anim.UpdateNodeImage(NodeList::GetNode(2), "mme.png");     // MME
//anim.UpdateNodeImage(NodeList::GetNode(3), "sgw.png");     // SGW
anim.UpdateNodeImage(4, enbImg);     // eNB-1
anim.UpdateNodeImage(5, enbImg);     // eNB-2
anim.UpdateNodeImage(6, enbImg);     // eNB-3
anim.UpdateNodeImage(7, ueImg);      // UE-1

anim.UpdateNodeSize(0, 50, 50);  // RemoteHost
anim.UpdateNodeSize(1, 10, 10);  // PGW
anim.UpdateNodeSize(2, 10, 10);  // MME
anim.UpdateNodeSize(3, 10, 10);  // SGW
anim.UpdateNodeSize(4, 50, 50);  // eNB-1
anim.UpdateNodeSize(5, 50, 50);  // eNB-2
anim.UpdateNodeSize(6, 50, 50);  // eNB-3
anim.UpdateNodeSize(7, 50, 50);  // UE-1


    std::string fileNameWithNoExtension = "TimeVSThroughput";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Time vs Throughput";
    std::string dataTitle               = "Throughput";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Time", "Throughput");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  allMon->CheckForLostPackets ();
  // call the flow monitor function
  ThroughputMonitor(&fmHelper, allMon, dataset); 
  
    Simulator::Run();

  //Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();
    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
