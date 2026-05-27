#!/usr/bin/env python
# -*- coding:utf-8 -*-
import os
import sys
import time
import Sniper
import CondDB
import DBISvc
import CalibSvc

def load_config(db_config_file):
    import yaml
    with open(db_config_file) as f:
        return yaml.safe_load(f)


def get_parser():
    import argparse
    parser = argparse.ArgumentParser(description='Run Tao Detector Simulation Analysis.')
    parser.add_argument('--loglevel', type=str, default='WARN',
                        choices=['DEBUG', 'INFO', 'WARN', 'ERROR'],
                        help='Log level (default: INFO)')
    parser.add_argument("--evtmax", type=int, default=10, help='events to be processed')
    parser.add_argument("--seed", type=int, default=42, help='seed')
    parser.add_argument("--charge_template_file", default = "charge_template", help='close dark noise')
    parser.add_argument("--input", default=None, nargs='+', help="specify input filename")
    # parser.add_argument("--inputType", default="calibAlg", choices=["calibAlg", "elecSim"], help="specify input type, calibAlg or elecSim") # 选择输入
    parser.add_argument("--use_true_vertex", default=False, help="use true vertex coordinate")
    parser.add_argument("--true_vertex_file", default='', help="input true vertex coordinate from Tbranch [Sim] in elecSim")
    parser.add_argument("--output", default="rec-output.root", help="specify output filename")
    parser.add_argument("--save-user-output", default=False, help="Specify whether to save user root file (true/false)")
    parser.add_argument("--user-output", default="user-ana-output.root", help="specify output filename")
    parser.add_argument("--algorithm", default="QMLERec", help="specify the analysis algorithm")
    parser.add_argument("--rec_Ge_only", default=False, help="reconstruct Ge68 events only")
    parser.add_argument("--Ge_evt_list_file", default='', help="input Ge68 event list file, only used when rec_Ge_only is set to true")
   
    ##CalibSvc
    parser.add_argument("--dbconf", default=None, help="Database Configuration (YAML format)")
    parser.add_argument('--DBcur', type=int, default=1764720000, help='Enter your name')
    parser.add_argument("--EnableUseCondDB", dest="EnableUseCondDB", action= 'store_true', help='Enable Use CondDB')
    # parser.add_argument("--DisableUseCondDB", dest="EnableUseCondDB", action='store_false')
    parser.set_defaults(EnableUseCondDB=True)

    ##CondDB
    parser.add_argument('--GTag', type=str, default="T25.7.4", help='Enter your name')
    parser.add_argument('--AutoUpdateMode', type=int, default=1, help='Enter your mode')
    parser.add_argument('--IovSinceType', type=str, default="TIMESTAMP", help='Enter your type')

    args = parser.parse_args()
    # parser.add_argument("--particle_PDG_ID", type=int, default= 11, help="specify particle PDG ID")
    ##
    # parser.add_argument("--enableChargeInfo", default=False, help="use nPE or Charge to reconstruct") # 默认将输入的电荷信息换算为PE数进行重建,最小化调用Chi2()；设置为true则不进行换算，直接使用电荷信息进行重,z建,最小化调用QMLE()..实际Chi2()和QMLE()的切换还有没有实现，需要在程序里手动修改

    return parser

if __name__ == "__main__":

    DATA_LOG_MAP = {"Test":0, "DEBUG":2, "INFO":3, "WARN":4, "ERROR":5, "Fatal":6}

    t0= time.time()
    parser = get_parser()
    args = parser.parse_args()
    print(args)

    Sniper.setLogLevel(DATA_LOG_MAP[args.loglevel])

    task = Sniper.Task("task")
    task.setEvtMax(args.evtmax)
    task.setLogLevel(DATA_LOG_MAP[args.loglevel])
    

    #task.setLogLevel(0)

    # = random svc =
    import RandomSvc
    rndm = task.createSvc("RandomSvc")
    rndm.property("Seed").set(args.seed)

    # = rootio =
    import RootIOSvc
    if not args.input:
        print("Please provide an input file for analysis")
        exit(-1)
    ris = task.createSvc("RootInputSvc/InputSvc")
    ris.property("InputFile").set(args.input)
    
    #if not args.true_vertex_file:
     #   print("Please provide an input file for particle id")
      #  exit(-1)
    # if not args.particle_PDG_ID:
    #     print("Please provide particle_PDG_ID")
    #     exit(-1)

    # = BufferMemMgr =
    import BufferMemMgr
    bufMgr = task.createSvc("BufferMemMgr")
    bufMgr.property("TimeWindow").set([0, 0])

    # = geometry service =
    import TAOGeometry
    simgeomsvc = task.createSvc("SimGeomSvc")

    import RootWriter
    if args.save_user_output:
        rootwriter = task.createSvc("RootWriter")
        rootwriter.property("Output").set({"RECEVT": args.user_output})

    task_writer = task.createSvc("RootOutputSvc/OutputSvc")
    task_writer.property("OutputStreams").set({
        "/Event/Rec/QMLE" : args.output
        })
    
    dbconf = args.dbconf
    if dbconf is None:
        # set to DBISVCROOT/share/dbi.yaml
        dbconf = os.environ.get("CALIBSVCROOT")
    dbconf = os.path.join(dbconf, "share", "dbi.yaml")

    if not os.path.exists(dbconf):
        print("ERROR: dbconf '%s' does not exist. "%dbconf)
        sys.exit(-1)
    db_config = load_config(dbconf)
    connections = db_config.get('connections')
    clients = db_config.get('clients')
    schemas = db_config.get('schemas')
    dbisvc = Sniper.create("SharedElem<DBISvc>/DBISvc")
    dbisvc.property("Connections").set(connections)
    dbisvc.property("Clients").set(clients)
    dbisvc.property("Schemas").set(schemas)
    task.addSvc(dbisvc)

    conddbsvc = Sniper.create("SharedElem<CondDBSvc>/CondDBSvc")
    conddbsvc.property("RepoTypes").set({
        #    "local": "LocalFSRepo",
        # "frontier": "Frontier"
        "dbi": "DBI"
    })
    conddbsvc.property("RepoURIs").set({
        #    "local": os.path.join("dummy-repo"),
        # "frontier": "http://junodb1.ihep.ac.cn:8080/Frontier",
        "dbi": "dbi://conddb" # configured by DBISvc
    })
    conddbsvc.property("GlobalTag").set(
        args.GTag
    )
    conddbsvc.property("AutoUpdateMode").set(args.AutoUpdateMode)
    conddbsvc.property("IovSinceType").set(args.IovSinceType)
    task.addSvc(conddbsvc)

    # Create CalibSvc
    calibsvc=task.createSvc("CalibSvc")
    calibsvc.property("EnableCondDB").set(args.EnableUseCondDB)
    calibsvc.property("DBcur").set(args.DBcur)



    # = ana detsim =
    Sniper.loadDll("libRecQMLEAlg.so")
    anadetsimalg = task.createAlg(args.algorithm)
    # if args.algorithm == "MakeChargeTemplate":
    #     anadetsimalg.property("TempRadius").set(args.temp_radius)
    #     anadetsimalg.property("CloseDarkNoise").set(True)
    # else:
    #     anadetsimalg.property("ChargeTemplateFile").set(args.charge_template_file)
    #     # if args.enableChargeInfo == "true":
    #     #     anadetsimalg.property("enableChargeInfo").set(True)
    #     # else:
    #     #     anadetsimalg.property("enableChargeInfo").set(False)
    #     # anadetsimalg.property("inputType").set(args.inputType)
    #     if args.use_true_vertex == "true":
    #         anadetsimalg.property("useTrueVertex").set(True)
    #     else:
    #         anadetsimalg.property("useTrueVertex").set(False)
    #     anadetsimalg.property("trueVertexFile").set(args.true_vertex_file)
    anadetsimalg.property("ChargeTemplateFile").set(args.charge_template_file)
    # anadetsimalg.property("particle_ID").set(args.particle_PDG_ID)
    if args.use_true_vertex == "true":
        anadetsimalg.property("useTrueVertex").set(True)
    else:
        anadetsimalg.property("useTrueVertex").set(False)
    anadetsimalg.property("trueVertexFile").set(args.true_vertex_file)
    # anadetsimalg.property("rec_Ge_only").set(args.rec_Ge_only)
    if args.rec_Ge_only == "true":
        anadetsimalg.property("rec_Ge_only").set(True)
    else:
        anadetsimalg.property("rec_Ge_only").set(False)
    anadetsimalg.property("Ge_evt_list_file").set(args.Ge_evt_list_file)
    anadetsimalg.property("saveUserFile").set(args.save_user_output)

    
    task.show()
    task.run()

    t1 = time.time() - t0
    print("===================================")
    print("run.py INFO: time elapsed: ", "{:10.4f}".format(t1), "s =", "{:10.4f}".format(t1/60), "min")
