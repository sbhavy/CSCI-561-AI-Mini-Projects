import csv, numpy as np, pandas as pd

class Linear:
	def __init__(self, inputD, outputD):
		self.params = dict()
		self.gradient = dict()

		limit = np.sqrt(2/(inputD + outputD))
		self.params['W'] = np.random.uniform(low = -limit, high = limit, size = (inputD, outputD))
		self.params['b'] = np.zeros((1, outputD))

		self.gradient['W'] = np.zeros((inputD, outputD))
		self.gradient['b'] = np.zeros((1, outputD))

	def forward(self, X): return X @ self.params['W'] + self.params['b']

	def backward(self, X, grad):
		self.gradient['W'] = X.T @ grad
		self.gradient['b'] = np.sum(grad, axis = 0)
		return grad @ self.params['W'].T

class ReLu:
	def forward(self, X): return X * (X > 0)
	def backward(self, X, grad): return grad * (X > 0)

class SoftmaxCE:
	def __init__(self):
		self.expand_Y = None
		self.calib_logit = None
		self.sum_exp_calib_logit = None
		self.prob = None

	def forward(self, X, Y):
		self.expand_Y = np.zeros(X.shape).reshape(-1)
		self.expand_Y[Y.astype(int).reshape(-1) + np.arange(X.shape[0]) * X.shape[1]] = 1.0
		self.expand_Y = self.expand_Y.reshape(X.shape)
		self.calib_logit = X - np.amax(X, axis = 1, keepdims = True)
		self.sum_exp_calib_logit = np.sum(np.exp(self.calib_logit), axis = 1, keepdims = True)
		self.prob = np.exp(self.calib_logit) / self.sum_exp_calib_logit

		return -np.sum(np.multiply(self.expand_Y, self.calib_logit - np.log(self.sum_exp_calib_logit))) / X.shape[0]

	def backward(self, X, Y): return -(self.expand_Y - self.prob) / X.shape[0]

def forward_pass(model, x):
	a1 = model['L1'].forward(x)
	h1 = model['H1'].forward(a1)
	a2 = model['L2'].forward(h1)
	h2 = model['H2'].forward(a2)
	a3 = model['L3'].forward(h2)

	return a1, h1, a2, h2, a3

def backward_pass(model, x, a1, h1, a2, h2, a3, y):
	loss = model['Loss'].forward(a3, y)
	grad_a3 = model['Loss'].backward(a3, y)
	grad_h2 = model['L3'].backward(h2, grad_a3)
	grad_a2 = model['H2'].backward(a2, grad_h2)
	grad_h1 = model['L2'].backward(h1, grad_a2)
	grad_a1 = model['H1'].backward(a1, grad_h1)
	grad_x = model['L1'].backward(x, grad_a1)

def model_update(model, learning_rate):
	for module_name, module in model.items():
		if hasattr(module, 'params'):
			for key, _ in module.params.items():
				module.params[key] -= module.gradient[key] * learning_rate
	return model

def predict_label(f):
    if f.shape[1] == 1: return (f > 0).astype(float)
    else: return np.argmax(f, axis=1).astype(float).reshape((f.shape[0], -1))

train_data = 'train_data.csv'
train_label = 'train_label.csv'
test_data = 'test_data.csv'

df = pd.read_csv(train_data)
df_res = pd.read_csv(train_label)
df = pd.concat([df, df_res], axis=1)
df = df[df.duplicated() == False]
df = df.drop(columns = ['BROKERTITLE', 'ADDRESS', 'STATE', 'MAIN_ADDRESS', 'ADMINISTRATIVE_AREA_LEVEL_2', 'LOCALITY', 'STREET_NAME', 'LONG_NAME', 'FORMATTED_ADDRESS'])

sublocs = df['SUBLOCALITY'].unique()
for i in range(len(sublocs)): sublocs[i] = sublocs[i].lower()
for col in sublocs: df[col] = df['SUBLOCALITY'].apply(lambda x: col in x.lower())

types = df['TYPE'].unique()
for i in range(len(types)): types[i] = types[i].lower()
for col in types: df[col] = df['TYPE'].apply(lambda x: col in x.lower())

Ytrain = df['BEDS'].to_numpy().astype('float64').reshape(-1,1)
df = df.drop(columns = ['BEDS', 'SUBLOCALITY', 'TYPE'])
Xtrain = df.to_numpy().astype('float64')

Q1 = np.percentile(Ytrain, 25)
Q3 = np.percentile(Ytrain, 75)
IQR = Q3 - Q1
lower_bound = Q1 - (1.5 * IQR)
upper_bound = Q3 + (1.5 * IQR)

idxs = np.where((Ytrain > lower_bound) & (Ytrain < upper_bound))[0]
Xtrain = Xtrain[idxs,:]
Ytrain = Ytrain[idxs]

mean = Xtrain.mean(axis = 0)
std = Xtrain.std(axis = 0)
Xtrain = (Xtrain - mean) / std

train_map, pred_map = {}, {}

temp = np.unique(Ytrain)

for i in range(len(temp)):
  train_map[temp[i]] = i
  pred_map[i] = temp[i]

for i in range(len(Ytrain)): Ytrain[i][0] = train_map[Ytrain[i][0]]

N_train, N_dims = Xtrain.shape[0], Xtrain.shape[1]
N_class = len(np.unique(Ytrain))
num_epoch = 200
learning_rate = 0.3

model = dict()
model['L1'] = Linear(N_dims, 256)
model['H1'] = ReLu()
model['L2'] = Linear(256, 64)
model['H2'] = ReLu()
model['L3'] = Linear(64, N_class)
model['Loss'] = SoftmaxCE()

for t in range(num_epoch):
	idxs = np.random.choice(N_train, 128, replace = False)
	x, y = Xtrain[idxs], Ytrain[idxs]
	a1, h1, a2, h2, a3 = forward_pass(model, x)
	backward_pass(model, x, a1, h1, a2, h2, a3, y)
	model = model_update(model, learning_rate)

df_test = pd.read_csv(test_data)

df_test = df_test.drop(columns = ['BROKERTITLE', 'ADDRESS', 'STATE', 'MAIN_ADDRESS', 'ADMINISTRATIVE_AREA_LEVEL_2', 'LOCALITY', 'STREET_NAME', 'LONG_NAME', 'FORMATTED_ADDRESS'])
for col in sublocs: df_test[col] = df_test['SUBLOCALITY'].apply(lambda x: col in x.lower())
for col in types: df_test[col] = df_test['TYPE'].apply(lambda x: col in x.lower())
df_test = df_test.drop(columns = ['SUBLOCALITY', 'TYPE'])
Xtest = df_test.to_numpy().astype('float64')
Xtest = (Xtest - mean) / std

with open('output.csv', 'w') as f:
	f.write('BEDS\n')
	for x in Xtest:
		_, _,_,_, a3 = forward_pass(model, x)
		pred = pred_map[predict_label(a3)[0][0]]
		f.write(str(int(pred))+'\n')