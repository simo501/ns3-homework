#include "ns3/applications-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/ssid.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("2060933");

int
main(int argc, char* argv[])
{
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////

    std::string studentId;
    bool isEnabledRtsCts = false;
    bool tracing = false;
    bool debug = false;

    CommandLine cmd;
    cmd.AddValue("studentId", "studentId number of the group representative", studentId);
    cmd.AddValue("enableRtsCts",
                 "boolean parameter to force the usage of RTS/CTS mechanism",
                 isEnabledRtsCts);
    cmd.AddValue("tracing", "boolean parameter to enable promiscuous mode", tracing);
    cmd.AddValue("debug", "boolean parameter to enable log on UdpEcho and OnOffApplication", debug);

    cmd.Parse(argc, argv);

    if (studentId.empty())
    {
        NS_LOG_UNCOND("missing studentId");
        return 1;
    }

    if (debug)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    }

    /*
        ===============================
            =POINT-TO-POINT HELPER=
        ===============================
    */

    PointToPointHelper ptp10_200;
    ptp10_200.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    ptp10_200.SetChannelAttribute("Delay", StringValue("200ms"));
    PointToPointStarHelper firstStar(2, ptp10_200);

    PointToPointHelper ptp100_20;
    ptp100_20.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    ptp100_20.SetChannelAttribute("Delay", StringValue("20ms"));

    PointToPointHelper ptp5_20;
    ptp5_20.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptp5_20.SetChannelAttribute("Delay", StringValue("20ms"));
    PointToPointStarHelper secondStar(2, ptp5_20);

    /*
        =============================
                =CSMA HELPER=
        =============================
    */

    CsmaHelper csma10_200;
    csma10_200.SetChannelAttribute("DataRate", StringValue("10Mbps"));
    csma10_200.SetChannelAttribute("Delay", TimeValue(MilliSeconds(200)));

    /////////////////////////////////////////////////////////////////////

    NodeContainer allNodes;

    NodeContainer rootNodeContainer;
    rootNodeContainer.Add(firstStar.GetHub());

    NodeContainer leavesNodeContainer;
    leavesNodeContainer.Add(firstStar.GetSpokeNode(0));
    leavesNodeContainer.Add(firstStar.GetSpokeNode(1));

    NodeContainer leaf2NodeContainer;
    leaf2NodeContainer.Create(1);

    NodeContainer root2NodeContainer;
    root2NodeContainer.Create(1);

    NodeContainer root4NodeContainer;
    root4NodeContainer.Create(1);

    NodeContainer leaves4NodeContainer;
    leaves4NodeContainer.Create(1);
    leaves4NodeContainer.Add(secondStar.GetHub());
    leaves4NodeContainer.Add(secondStar.GetSpokeNode(0));
    leaves4NodeContainer.Add(secondStar.GetSpokeNode(1));

    // there is no 3 because 3 is represented by the AP
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Add(wifiApNode);
    wifiStaNodes.Create(9);

    NetDeviceContainer mainSubnet;
    NetDeviceContainer firstSubnet;
    NetDeviceContainer secondSubnet;
    NetDeviceContainer fourthSubnet; // third subnet == wifi

    // DON'T TOUCH THE ORDER
    allNodes.Add(leavesNodeContainer);  //  2
    allNodes.Add(rootNodeContainer);    //  1
    allNodes.Add(leaf2NodeContainer);   //  1
    allNodes.Add(root2NodeContainer);   //  1
    allNodes.Add(root4NodeContainer);   //  1
    allNodes.Add(leaves4NodeContainer); //  4
    // there is no access point because it's in adhoc mode
    // allNodes.Add(wifiApNode);           //  1
    allNodes.Add(wifiStaNodes); //  9 + 1

    // internet stack
    InternetStackHelper stack;
    stack.Install(allNodes);

    //////////////////////////////////////////////////////////////////////////

    // main nodes netdevicecontainer
    mainSubnet.Add(ptp100_20.Install(rootNodeContainer.Get(0), root2NodeContainer.Get(0)));
    mainSubnet.Add(ptp100_20.Install(wifiApNode.Get(0), root2NodeContainer.Get(0)));
    mainSubnet.Add(ptp100_20.Install(root2NodeContainer.Get(0), root4NodeContainer.Get(0)));

    //////////////////////////////////////////////////////////////////////

    // second subnet
    NodeContainer secondSubnetCsmaNodeContainer;
    secondSubnetCsmaNodeContainer.Add(root2NodeContainer);
    secondSubnetCsmaNodeContainer.Add(leaf2NodeContainer);

    secondSubnet.Add(csma10_200.Install(secondSubnetCsmaNodeContainer));

    // fourth subnet
    fourthSubnet.Add(
        ptp5_20.Install(root4NodeContainer.Get(0), leaves4NodeContainer.Get(0))); // first leaf
    fourthSubnet.Add(ptp5_20.Install(root4NodeContainer.Get(0),
                                     secondStar.GetHub())); // second leaf (the star one)

    // WIFI PART
    ////////////////////////////////////////////////////////////////
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;

    // Ad Hoc Mode
    NetDeviceContainer adhocDevices;
    mac.SetType("ns3::AdhocWifiMac");
    adhocDevices = wifi.Install(phy, mac, wifiStaNodes);

    if (isEnabledRtsCts)
    {
        NS_LOG_UNCOND("Request to Send / Clear to Send is enabled.");
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));
    }

    Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator>();
    positionAllocS->Add(Vector(-10.0, -10.0, 0.0)); // node
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAllocS);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode);

    // IP ASSIGNMENT
    //////////////////////////////////////////////////////

    Ipv4AddressHelper addresses;

    // main nodes addresses
    addresses.SetBase("10.0.0.0", "255.255.255.248"); //    so we have 8 address / 29
    Ipv4InterfaceContainer mainSubnetInterfaces = addresses.Assign(mainSubnet);
    // first subnet addresses
    addresses.SetBase("10.0.1.0", "255.255.255.248");
    firstStar.AssignIpv4Addresses(addresses);
    // second subnet addresses
    addresses.SetBase("10.0.2.0", "255.255.255.252");
    Ipv4InterfaceContainer secondSubnetInterfaces = addresses.Assign(secondSubnet);
    // wifi addresses
    addresses.SetBase("10.0.3.0", "255.255.255.224");
    Ipv4InterfaceContainer staInterfaces = addresses.Assign(adhocDevices);

    // fourth subnet addresses
    addresses.SetBase("10.0.4.0", "255.255.255.240");
    Ipv4InterfaceContainer fourthSubnetInterfaces = addresses.Assign(fourthSubnet);
    // fifith subnet addresses
    // addresses.SetBase("10.0.5.0", "255.255.255.248");
    // secondStar.AssignIpv4Addresses(addresses);

    // ECHO    server 15 client 3
    uint16_t port = 9; // well-known echo port number
    UdpEchoServerHelper server(port);
    ApplicationContainer serverApp = server.Install(secondSubnetCsmaNodeContainer.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(15.0));

    uint32_t packetSize = 1169;

    // the ip address of the udp echo server it's what is needed here
    UdpEchoClientHelper client(secondSubnetInterfaces.GetAddress(1), port);

    client.SetAttribute("MaxPackets", UintegerValue(250));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(20)));

    uint8_t packetData[] = {
        67,  104, 114, 105, 115, 116, 105, 97,  110, 44,  83,  97,  118, 105, 110, 105, 44,
        50,  48,  54,  48,  57,  51,  51,  44,  69,  109, 97,  110, 117, 101, 108, 101, 44,
        77,  117, 114, 105, 110, 111, 44,  50,  48,  54,  48,  53,  53,  53,  44,  77,  97,
        116, 116, 101, 111, 44,  77,  97,  122, 122, 97,  44,  50,  48,  53,  52,  53,  51,
        52,  44,  78,  105, 99,  99,  111, 108, 195, 178, 44,  80,  111, 122, 105, 111, 44,
        50,  48,  56,  53,  53,  49,  50,  44,  83,  105, 109, 111, 110, 101, 44,  80,  97,
        110, 100, 111, 108, 102, 105, 44,  50,  48,  56,  53,  55,  48,  51};

    ApplicationContainer clientApp = client.Install(wifiStaNodes.Get(5));

    client.SetFill(clientApp.Get(0), packetData, sizeof(packetData), packetSize);

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(15.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(15.0));

    // PACKET SINK HELPER
    Address firstOnOffReceiverAddr(InetSocketAddress(firstStar.GetHubIpv4Address(0), 50000));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", firstOnOffReceiverAddr);
    ApplicationContainer serverOnOff1 = packetSinkHelper.Install(rootNodeContainer.Get(0));
    serverOnOff1.Start(Seconds(0.0));
    serverOnOff1.Stop(Seconds(15.0));

    Address secondOnOffReceiverAddr(InetSocketAddress(firstStar.GetSpokeIpv4Address(0), 50001));
    PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", secondOnOffReceiverAddr);
    ApplicationContainer serverOnOff2 = packetSinkHelper2.Install(leavesNodeContainer.Get(0));
    serverOnOff2.Start(Seconds(0.0));
    serverOnOff2.Stop(Seconds(15.0));

    // 6 ------> 0
    ApplicationContainer appContainerSender6Receiver0;
    OnOffHelper onOffHelperSender6Receiver0("ns3::TcpSocketFactory", Address());
    onOffHelperSender6Receiver0.SetAttribute("OnTime",
                                             StringValue("ns3::ExponentialRandomVariable[Mean=1]"));
    onOffHelperSender6Receiver0.SetAttribute("OffTime",
                                             StringValue("ns3::ExponentialRandomVariable[Mean=1]"));

    AddressValue sender6Receiver0OnOffAddr(
        InetSocketAddress(firstStar.GetSpokeIpv4Address(0), 50001));
    onOffHelperSender6Receiver0.SetAttribute("Remote", sender6Receiver0OnOffAddr);
    onOffHelperSender6Receiver0.SetAttribute("PacketSize", UintegerValue(1077));
    appContainerSender6Receiver0 = onOffHelperSender6Receiver0.Install(secondStar.GetHub());
    appContainerSender6Receiver0.Start(Seconds(0.73));
    appContainerSender6Receiver0.Stop(Seconds(15.0));

    // 12 ------> 1
    ApplicationContainer appContainerSender12Receiver1;
    OnOffHelper onOffHelperSender12Receiver1("ns3::TcpSocketFactory", Address());
    onOffHelperSender12Receiver1.SetAttribute(
        "OnTime",
        StringValue("ns3::ExponentialRandomVariable[Mean=1]"));
    onOffHelperSender12Receiver1.SetAttribute(
        "OffTime",
        StringValue("ns3::ExponentialRandomVariable[Mean=1]"));

    AddressValue sender12Receiver1OnOffAddr(
        InetSocketAddress(firstStar.GetHubIpv4Address(0), 50000));
    onOffHelperSender12Receiver1.SetAttribute("Remote", sender12Receiver1OnOffAddr);
    onOffHelperSender12Receiver1.SetAttribute("PacketSize", UintegerValue(1331));
    appContainerSender12Receiver1 = onOffHelperSender12Receiver1.Install(wifiStaNodes.Get(2));
    appContainerSender12Receiver1.Start(Seconds(3.15));
    appContainerSender12Receiver1.Stop(Seconds(15.0));

    // 13 ------> 0
    ApplicationContainer appContainerSender13Receiver0;
    OnOffHelper onOffHelperSender13Receiver0("ns3::TcpSocketFactory", Address());
    onOffHelperSender13Receiver0.SetAttribute(
        "OnTime",
        StringValue("ns3::ExponentialRandomVariable[Mean=1]"));
    onOffHelperSender13Receiver0.SetAttribute(
        "OffTime",
        StringValue("ns3::ExponentialRandomVariable[Mean=1]"));

    AddressValue sender13Receiver0OnOffAddr(
        InetSocketAddress(firstStar.GetSpokeIpv4Address(0), 50001));
    onOffHelperSender13Receiver0.SetAttribute("Remote", sender13Receiver0OnOffAddr);
    onOffHelperSender13Receiver0.SetAttribute("PacketSize", UintegerValue(1749));
    appContainerSender13Receiver0 = onOffHelperSender13Receiver0.Install(wifiStaNodes.Get(3));
    appContainerSender13Receiver0.Start(Seconds(3.35));
    appContainerSender13Receiver0.Stop(Seconds(15.0));

    if (tracing)
    {
        NS_LOG_UNCOND("Tracing is enabled.");
        ptp100_20.EnablePcap("pcap/task-2", rootNodeContainer, true);
        ptp100_20.EnablePcap("pcap/task-10", wifiApNode, true);
        ptp100_20.EnablePcap("pcap/task-6", root4NodeContainer, true);
        ptp100_20.EnablePcap("pcap/task-4", root2NodeContainer, true);
    }

    NS_LOG_INFO("STARTING SIMULATION...");
    Simulator::Run();
    Simulator::Stop(Seconds(15));
    Simulator::Destroy();
    NS_LOG_INFO("DONE.");

    return 0;
}
