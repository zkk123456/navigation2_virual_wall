import rclpy
from rclpy.node import Node
from beefast_interfaces.srv import ManageDb
import json
import argparse

class ServiceClient(Node):
    def __init__(self):
        super().__init__('service_client')
        self.client = self.create_client(ManageDb, '/database/service')
        self.wait_for_service(self.client)

    def wait_for_service(self, client):
        while not client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('DB Service not available, waiting again...')

    def call_service(self, request_json):
        req = ManageDb.Request()
        req.request_data = request_json

        future = self.client.call_async(req)
        rclpy.spin_until_future_complete(self, future)

        if future.result() is not None:
            response = future.result()
            self.get_logger().info(f'op:{request_json}, Response: {response.response_data}')
            return response
        else:
            self.get_logger().error('Failed to call service')
            return None

def main(opt, args=None):
    rclpy.init(args=args)
    service_client = ServiceClient()
    if opt.op == 'getallmap':
        # 发送getallmap服务的请求消息
        request_json = json.dumps({"op":"getallmap"})
        response = service_client.call_service(request_json)
    elif opt.op == 'getmap':
        # 测试getmap
        request_json = json.dumps({"op":"getmap","map_id":8})
        response = service_client.call_service(request_json)
    elif opt.op == 'renamemap':
        # # 重命名地图
        request_json = json.dumps({"op":"updatemap","map_id":3, "name":"map001_2014_11_25"})
        response = service_client.call_service(request_json)
    elif opt.op == 'addmap':
        # # 新增地图
        request_json = json.dumps({"op":"addmap","name":"map06_2024_11", "file_name":"map006_2024_11_26"})
        response = service_client.call_service(request_json)
    elif opt.op == 'deletemap':
        # 删除地图
        request_json = json.dumps({"op":"deletemap","map_id":8})
        response = service_client.call_service(request_json)
    elif opt.op == 'addroom':
        # 增加房间
        # request_json = json.dumps({"op":"addroom",
        #                            "data":[{"map_id":8, "pix_value":1, "name":"房间1"},
        #                                    {"map_id":8, "pix_value":2, "name":"房间2"},
        #                                    {"map_id":8, "pix_value":3, "name":"房间3"},]})
        request_json = json.dumps({"op":"addroom",
                            "data":[{"map_id":8, "room_value":4, "room_name":"房间4"}]})
        response = service_client.call_service(request_json)
    elif opt.op == 'getroom':
        # 增加房间
        request_json = json.dumps({"op":"getroom","map_id":8})
        response = service_client.call_service(request_json)
    elif opt.op == 'updateroom':
        # 重命名房间        
        request_json = json.dumps({"op":"updateroom","map_id":8, 'room_value':2, "room_name":"主卧"})
        response = service_client.call_service(request_json)  
    elif opt.op == 'deleteroom':
        # 合并房间,需要删除房间,保留面积大的
        request_json = json.dumps({"op":"deleteroom","map_id":8, 'room_value':2})
        response = service_client.call_service(request_json)      
    elif opt.op == 'addzone':       
        request_json = json.dumps({"op":"addzone",
                            "data":[{"map_id":0, "zone_type":4, "zone_coordinates":"(0 0)(1.0 0)(1.0 1.0)(0 1.0)"},
                                    {"map_id":0, "zone_type":2, "zone_coordinates":"(1.0 0)(0 0)(0 1.0)"}
                                    ]})
        response = service_client.call_service(request_json)        
    elif opt.op == 'getzone': 
        # 增加房间
        #"data": {"map_id":8}
        request_json = json.dumps({"op":"getzone","data": {"map_id":0}})
        response = service_client.call_service(request_json)
    elif opt.op == 'deletezone':
        # 删除指定地图的指定虚拟墙信息
        request_json = json.dumps({"op":"deletezone","data": {"map_id":0,"zone_id":2}})
        response = service_client.call_service(request_json) 
    elif opt.op == 'deletezones':
        # 删除指定地图的指定虚拟墙信息
        request_json = json.dumps({"op":"deletezones","data": {"map_id":0}})
        response = service_client.call_service(request_json) 
    
    service_client.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--op', type=str, default='', help='operator')
    opt = parser.parse_args()
    main(opt)