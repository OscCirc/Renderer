
## Issues in transformation
### Rodrigues' rotation formula
用于计算三维空间中一个向量绕着任意单位轴旋转给定角度后，所得到的新向量的位置。假设我们有一个三维空间中的向量 $\mathbf{v}$，我们要将其绕着一个单位向量 $\mathbf{k}$（代表旋转轴）按照右手定则旋转 $\theta$ 角度。旋转后的新向量 $\mathbf{v}_{rot}$ 可以通过以下公式求得：
$$\mathbf{v}_{rot} = \mathbf{v} \cos\theta + (\mathbf{k} \times \mathbf{v}) \sin\theta + \mathbf{k} (\mathbf{k} \cdot \mathbf{v}) (1 - \cos\theta)$$
[To learn more in wiki](https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula)

### Linear transformation and translation
旋转和平移的先后次序不同影响最终变换的结果。

解释：

- 矩阵运算不满足交换律：
$$M_T M_A = \begin{bmatrix} I & \mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix} \begin{bmatrix} A & \mathbf{0} \\ \mathbf{0}^T & 1 \end{bmatrix} = \begin{bmatrix} A & \mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix}$$
而
$$M_A M_T = \begin{bmatrix} A & \mathbf{0} \\ \mathbf{0}^T & 1 \end{bmatrix} \begin{bmatrix} I & \mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix} = \begin{bmatrix} A & A\mathbf{t} \\ \mathbf{0}^T & 1 \end{bmatrix}$$
或者
$$A\mathbf{x} + \mathbf{t}\neq A(\mathbf{x} + \mathbf{t})$$
[Click this to learn more](#SVD)

### MVP
Model-View-Projection 矩阵对应了三个几何计算阶段（从矩阵计算的角度其实是PVM）。

View Matrix 对应先平移、后变换的坐标系转换操作：
$$
\begin{aligned}
V = R_{view} \cdot T_{view} = \begin{bmatrix} \mathbf{x}^T & 0 \\ \mathbf{y}^T & 0 \\ \mathbf{z}^T & 0 \\ 0 & 1 \end{bmatrix} \begin{bmatrix} I & -\mathbf{e} \\ 0 & 1 \end{bmatrix}
\end{aligned}
$$
为了方便理解，$R_{view}$也可视为对相机坐标系三个坐标轴的投影矩阵。

Projection Matrix 
$$ M_{proj} = \begin{bmatrix}
\frac{2n}{r - l} & 0 & \frac{r + l}{r - l} & 0 \\
0 & \frac{2n}{t - b} & \frac{t + b}{t - b} & 0 \\
0 & 0 & -\frac{f + n}{f - n} & -\frac{2fn}{f - n} \\
0 & 0 & -1 & 0
\end{bmatrix} $$

为什么$M_{43} = -1$ ？考虑到透视投影中$xy$坐标变换涉及除以$z$，因而需要通过齐次坐标的第四个分量来储存，$M_{proj}$的第四行就是为此设置的。 

## Suppelementary

### When equal?
<a id = "SVD"></a>
一个有趣的讨论是什么时候交换二者结果一致？

众所周知，任何线性变换都可以分解为旋转和缩放的组合：
$$A = U \Sigma V^T$$
将 $A\mathbf{t} = \mathbf{t}$ 代入 SVD 的形式中：
$$
\begin{aligned}
& U \Sigma V^T \mathbf{t} = \mathbf{t}\\

\rightarrow & \Sigma (V^T \mathbf{t}) = U^T \mathbf{t}

\end{aligned}
$$
两端取模可得
$$\|\Sigma (V^T \mathbf{t})\| = \|\mathbf{t}\|$$
这意味着$t$参与的所有主轴对应的奇异值必为$1$，也即不能对$t$参与的任何一个维度拉伸，从而有：
$$\Sigma (V^T \mathbf{t}) = V^T \mathbf{t}$$
即
$$U V^T \mathbf{t} = \mathbf{t}$$
而这又意味着不能对$t$参与的任何一个维度旋转（或称 $t$ 垂直于$UV^T$所代表的旋转平面）。