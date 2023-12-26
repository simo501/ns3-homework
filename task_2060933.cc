#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("2060933");

int
main(int argc, char* argv[])
{
    /////////////////////////////////////////////////////////////////
    // LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    /////////////////////////////////////////////////////////////////

    std::string studentId;
    bool isEnabledRtsCts = false;
    bool tracing = false;

    CommandLine cmd;
    cmd.AddValue("studentId", "studentId number of the group representative", studentId);
    cmd.AddValue("enableRtsCts",
                 "boolean parameter to force the usage of RTS/CTS mechanism",
                 isEnabledRtsCts);
    cmd.AddValue("tracing", "boolean parameter to enable promiscuous mode", tracing);

    cmd.Parse(argc, argv);

    if (studentId.empty())
    {
        NS_LOG_UNCOND("missing studentId");
        return -1;
    }

    NodeContainer allNodes;

    NodeContainer rootNodeContainer;
    rootNodeContainer.Create(1);

    NodeContainer leavesNodeContainer;
    leavesNodeContainer.Create(2);

    NodeContainer leaf2NodeContainer;
    leaf2NodeContainer.Create(1);

    NodeContainer root2NodeContainer;
    root2NodeContainer.Create(1);

    NodeContainer root4NodeContainer;
    root4NodeContainer.Create(1);

    NodeContainer leaves4NodeContainer;
    leaves4NodeContainer.Create(4);

    // there is no 3 because 3 is represented by the AP
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(9);

    // NodeContainer root1_2NodeContainer;
    // root1_2NodeContainer.Add(rootNodeContainer);
    // root1_2NodeContainer.Add(root2NodeContainer);

    // NodeContainer root2_3NodeContainer;
    // root2_3NodeContainer.Add(root2NodeContainer);
    // root2_3NodeContainer.Add(wifiApNode);

    // NodeContainer root2_4NodeContainer;
    // root2_4NodeContainer.Add(root2NodeContainer);
    // root2_4NodeContainer.Add(root4NodeContainer);

    NetDeviceContainer mainNodes;
    NetDeviceContainer firstSubnet;
    NetDeviceContainer secondSubnet;
    NetDeviceContainer fourthSubnet; // third subnet == wifi

    // DON'T TOUCH THE ORDER
    allNodes.Add(rootNodeContainer);    //  1
    allNodes.Add(leavesNodeContainer);  //  2
    allNodes.Add(leaf2NodeContainer);   //  1
    allNodes.Add(root2NodeContainer);   //  1
    allNodes.Add(root4NodeContainer);   //  1
    allNodes.Add(leaves4NodeContainer); //  4
    allNodes.Add(wifiApNode);           //  1
    allNodes.Add(wifiStaNodes);         //  9

    // internet stack
    InternetStackHelper stack;
    stack.Install(allNodes);

    /*

        POINT-TO-POINT HELPER

    */

    PointToPointHelper ptp10_200;
    ptp10_200.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    ptp10_200.SetChannelAttribute("Delay", StringValue("200ms"));

    PointToPointHelper ptp100_20;
    ptp100_20.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    ptp100_20.SetChannelAttribute("Delay", StringValue("20ms"));

    PointToPointHelper ptp5_20;
    ptp5_20.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    ptp5_20.SetChannelAttribute("Delay", StringValue("20ms"));

    /*

        CSMA HELPER

    */

    CsmaHelper csma100_200;
    csma100_200.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma100_200.SetChannelAttribute("Delay", TimeValue(MilliSeconds(200)));

    /////////////////////////////////////////////////////////////////////

    // main nodes netdevicecontainer
    mainNodes.Add(ptp100_20.Install(rootNodeContainer.Get(0), root2NodeContainer.Get(0)));
    mainNodes.Add(ptp100_20.Install(root2NodeContainer.Get(0), wifiApNode.Get(0)));
    mainNodes.Add(ptp100_20.Install(root2NodeContainer.Get(0), root4NodeContainer.Get(0)));

    // main nodes addresses
    Ipv4AddressHelper addresses;

    addresses.SetBase("10.0.0.0", "255.255.255.248"); //    so we have 8 address / 29
    Ipv4InterfaceContainer mainNodesInterfaces = addresses.Assign(mainNodes);

    // first subnet
    firstSubnet.Add(ptp10_200.Install(rootNodeContainer.Get(0), leavesNodeContainer.Get(0)));
    firstSubnet.Add(ptp10_200.Install(rootNodeContainer.Get(0), leavesNodeContainer.Get(1)));

    // first subnet addresses
    addresses.SetBase("10.0.1.0", "255.255.255.248");
    Ipv4InterfaceContainer firstSubnetInterfaces = addresses.Assign(firstSubnet);

    // second subnet
    NodeContainer secondSubnetCsmaNodeContainer;
    secondSubnetCsmaNodeContainer.Add(root2NodeContainer);
    secondSubnetCsmaNodeContainer.Add(leaf2NodeContainer);

    secondSubnet.Add(csma100_200.Install(secondSubnetCsmaNodeContainer));

    // second subnet addresses
    addresses.SetBase("10.0.2.0", "255.255.255.252");
    Ipv4InterfaceContainer secondSubnetInterfaces = addresses.Assign(secondSubnet);

    // fourth subnet
    fourthSubnet.Add(ptp5_20.Install(root4NodeContainer.Get(0), leaves4NodeContainer.Get(0)));
    fourthSubnet.Add(ptp5_20.Install(root4NodeContainer.Get(0), leaves4NodeContainer.Get(1)));
    fourthSubnet.Add(ptp5_20.Install(leaves4NodeContainer.Get(0), leaves4NodeContainer.Get(2)));
    fourthSubnet.Add(ptp5_20.Install(leaves4NodeContainer.Get(0), leaves4NodeContainer.Get(3)));

    addresses.SetBase("10.0.4.0", "255.255.255.240");
    Ipv4InterfaceContainer fourthSubnetInterfaces = addresses.Assign(fourthSubnet);

    // wifi
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

    Ssid ssid = Ssid("2060933");

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiApNode);

    // wifi addresses
    addresses.SetBase("10.0.3.0", "255.255.255.224");
    Ipv4InterfaceContainer staInterfaces = addresses.Assign(adhocDevices);
    Ipv4InterfaceContainer apInterfaces = addresses.Assign(apDevices);

    // Ipv4InterfaceContainer allInterfaces;
    // allInterfaces.Add(mainNodesInterfaces);
    // allInterfaces.Add(firstSubnetInterfaces);
    // allInterfaces.Add(secondSubnetInterfaces);
    // allInterfaces.Add(fourthSubnetInterfaces);
    // allInterfaces.Add(apInterfaces);
    // allInterfaces.Add(staInterfaces);

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    uint16_t port = 9; // well-known echo port number
    UdpEchoServerHelper server(port);
    ApplicationContainer serverApp = server.Install(allNodes.Get(4));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    uint32_t packetSize = 1169;
    uint32_t maxPacketCount = 250;

    Time interPacketInterval = MilliSeconds(20);

    UdpEchoClientHelper client(staInterfaces.GetAddress(5), port);

    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    // client.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer clientApp = client.Install(allNodes.Get(16));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    uint8_t packetData[] = {
        67,  104, 114, 105, 115, 116, 105, 97,  110, 44,  83,  97,  118, 105, 110, 105, 44,
        50,  48,  54,  48,  57,  51,  51,  44,  69,  109, 97,  110, 117, 101, 108, 101, 44,
        77,  117, 114, 105, 110, 111, 44,  50,  48,  54,  48,  53,  53,  53,  44,  77,  97,
        116, 116, 101, 111, 44,  77,  97,  122, 122, 97,  44,  50,  48,  53,  52,  53,  51,
        52,  44,  78,  105, 99,  99,  111, 108, 195, 178, 44,  80,  111, 122, 105, 111, 44,
        50,  48,  56,  53,  53,  49,  50,  44,  83,  105, 109, 111, 110, 101, 44,  80,  97,
        110, 100, 111, 108, 102, 105, 44,  50,  48,  56,  53,  55,  48,  51};

    client.SetFill(clientApp.Get(0), packetData, sizeof(packetData), packetSize);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(15.0));

    OnOffHelper onOffHelperSender6Receiver0("ns3::TcpSocketFactory", Address());
    onOffHelperSender6Receiver0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelperSender6Receiver0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    AddressValue firstOnOffAddr(InetSocketAddress(firstSubnetInterfaces.GetAddress(1), port));
    onOffHelperSender6Receiver0.SetAttribute("Remote", firstOnOffAddr);
    clientApp.Add(onOffHelperSender6Receiver0.Install(leaves4NodeContainer.Get(0)));

    // to print ip addresses of subnets
    // for (int i = 0; i < (int)firstSubnetInterfaces.GetN(); i++ ) {
    //     NS_LOG_UNCOND("IP " << i << " : " << firstSubnetInterfaces.GetAddress(i));
    // }

    // for (int i = 0; i < (int)mainNodesInterfaces.GetN(); i++ ) {
    //     NS_LOG_UNCOND("IP " << i << " : " << mainNodesInterfaces.GetAddress(i));
    // }

    /* For loop to check interfaces, nodes and applications */
    ///////////////////////////////////////////////////////////
    // for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
    // {
    //     for (uint32_t j = 0; j < NodeList::GetNode(i)->GetNDevices(); j++)
    //     {
    //         Ptr<NetDevice> device = NodeList::GetNode(i)->GetDevice(j);
    //         NS_LOG_UNCOND("Node " << i << ", Device " << j
    //                               << ", Address: " << device->GetAddress());
    //     }

    //     for (uint32_t z = 0; z < NodeList::GetNode(i)->GetNApplications(); z++)
    //     {
    //         Ptr<Application> app = NodeList::GetNode(i)->GetApplication(z);
    //         NS_LOG_UNCOND("Node " << i << ", Application " << z
    //                               << ", Type: " << app->GetInstanceTypeId());
    //     }
    // }
    ////////////////////////////////////////////////////////////

    if (tracing)
    {
    }

    NS_LOG_INFO("STARTING SIMULATION...");
    Simulator::Run();
    Simulator::Stop(Seconds(10));
    Simulator::Destroy();
    NS_LOG_INFO("DONE.");

    return 0;
}
