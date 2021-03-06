#!/usr/bin/env python3

import os
import sys
import pprint

import numpy as np
import multiprocessing as mp

import torch
from torch.utils.data import Dataset, DataLoader

import torchvision
import torchvision.transforms as transforms

from PIL import Image

import utils
import timeit
#import yolov3 as yolo
import yolov3_compress as yolo

device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

def parse_labels(label, size):
    labels = []
    for line in label.split("\n")[:-1]:
        data = line.strip().split(' ')
        bb_o = [float(x) for x in data[1:]]
        idx = int(data[0])

        bb = [0] * 4
        bb[0] = size * (bb_o[0] - bb_o[2] / 2)
        bb[1] = size * (bb_o[1] - bb_o[3] / 2)
        bb[2] = size * (bb_o[0] + bb_o[2] / 2)
        bb[3] = size * (bb_o[1] + bb_o[3] / 2)

        labels += [{
            'coco_idx': idx,
            'bb': bb
        }]

    return labels

class YoloDataset(Dataset):
    def __init__(self, images_path, labels_path, transforms, size=416):
        self.images = []
        self.labels = []
        self.images_path = images_path
        self.labels_path = labels_path
        self.size = size
        self.max_items = 50

        self.transforms = transforms

        for img in sorted(os.listdir(images_path)):
            lbl = os.path.basename(img).split('.')[0] + '.txt'
            label_path = os.path.join(labels_path, lbl)

            if os.path.exists(label_path):
                self.images += [img]
                self.labels += [lbl]

        assert(len(self.images) == len(self.labels))

    def __len__(self):
        return len(self.images)

    def __getitem__(self, index):
        image_path = os.path.join(self.images_path, self.images[index])
        label_path = os.path.join(self.labels_path, self.labels[index])

        labels = []
        with open(label_path) as label_file:
            labels += [label_file.read()]

        image_original = Image.open(image_path).convert('RGB')
        image_original = image_original.resize((self.size, self.size))
        image = np.asarray(image_original)

        image = self.transforms(image)

        return image, labels

def main(images_path, labels_path):
    size = 416

    t = transforms.Compose([
        transforms.ToTensor(),
        #transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
    ])

    data = DataLoader(YoloDataset(images_path, labels_path, t, size), batch_size=64, shuffle=True, num_workers=mp.cpu_count())
    model = yolo.load_model()
    model.to(device)

    count = [0] * 80
    correct = [0] * 80

    from nnfc.modules.nnfc import CompressionLayer
    # image_compression_layer = CompressionLayer(encoder_name='jpeg_image_encoder',
    #                                            encoder_params_dict={'quantizer' : 100},
    #                                            decoder_name='jpeg_image_decoder',
    #                                            decoder_params_dict={})

    # image_compression_layer = CompressionLayer(encoder_name='h264_image_encoder',
    #                                            encoder_params_dict={'quantizer' : 29},
    #                                            decoder_name='h264_image_decoder',
    #                                            decoder_params_dict={}) 

    # image_compression_layer = CompressionLayer(encoder_name='h265_image_encoder',
    #                                            encoder_params_dict={'quantizer' : 1},
    #                                            decoder_name='h265_image_decoder',
    #                                            decoder_params_dict={}) 

    # image_compression_layer = CompressionLayer(encoder_name='rgbswizzler_encoder',
    #                                            encoder_params_dict={},
    #                                            decoder_name='rgbswizzler_decoder',
    #                                            decoder_params_dict={}) 

    compressed_sizes = []
    with torch.no_grad():
        for i, (local_batch, local_labels) in enumerate(data):

            local_batch = local_batch.to(device)
            # local_batch = image_compression_layer(local_batch)
            output = model(local_batch)
            compressed_sizes += model.compression_layer.get_compressed_sizes()
            # compressed_sizes += image_compression_layer.get_compressed_sizes()
            print(compressed_sizes[-3:])

            for j, detections in enumerate(utils.parse_detections(output)):
                detections = utils.non_max_suppression(detections, confidence_threshold=0.2)

                for target in parse_labels(local_labels[0][j], size):

                    count[target['coco_idx']] += 1
                    for det in [det for det in detections if det.coco_idx == target['coco_idx']]:
                        if utils.iou(target['bb'], det.bb) >= 0.5:
                            correct[target['coco_idx']] += 1
                            break

            psum = 0
            for j in range(80):
                if count[j] == 0:
                    psum = -80
                    break

                p = correct[j] / count[j]
                psum += p

            print('[%d/%d] mAP: %.6f (%.6f)' % (i + 1, len(data), psum / 80, sum(correct)/sum(count)))

        print('final!', np.mean(np.asarray(compressed_sizes)))

        
if __name__ == '__main__':
    main(images_path=sys.argv[1], labels_path=sys.argv[2])
