# functions/add.py
import torch
from torch.autograd import Function
from .._ext import mfc_wrapper

import timeit
import sys

_DEBUG = True

class NoopEncoderFunc(Function):

    @staticmethod
    def forward(ctx, inp, mem1, mem2, gpu):

        # copy tensor to the CPU if necessary
        t1 = timeit.default_timer()

        if gpu:
            mfc_wrapper.device_to_host_copy(mem1, inp)
            inp = mem1

        t2 = timeit.default_timer()
        if _DEBUG:
            sys.stderr.write('copy to host time: {}\n'.format(t2-t1))

        # perform the computation
        t1 = timeit.default_timer()

        mfc_wrapper.noop_encode_forward(inp, mem2)

        t2 = timeit.default_timer()
        if _DEBUG:
            sys.stderr.write('encode time: {}\n'.format(t2-t1))

        return mem2

    @staticmethod
    def backward(ctx, grad_output):
        #grad_input = grad_output.new()
        #mfc_wrapper.noop_encode_backward(grad_output, grad_input)
        return grad_output, None, None, None

    
class NoopDecoderFunc(Function):

    @staticmethod
    def forward(ctx, inp, mem1, gpu):

        t1 = timeit.default_timer()
        mfc_wrapper.noop_decode_forward(inp, mem1)
        t2 = timeit.default_timer()
        if _DEBUG:
            sys.stderr.write('decode time: {}\n'.format(t2-t1))

        # copy the data to the gpu isf necessary    
        if gpu:
            t1 = timeit.default_timer()
            mem1 = mem1.cuda()
            t2 = timeit.default_timer()
            if _DEBUG:
                sys.stderr.write('copy to gpu time: {}\n\n'.format(t2-t1))

        return mem1
        
    @staticmethod
    def backward(ctx, grad_output):
        #grad_input = grad_output.new()
        #mfc_wrapper.noop_decode_backward(grad_output, grad_input)
        return grad_output, None, None
    
