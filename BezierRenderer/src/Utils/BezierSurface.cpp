#include "BezierSurface.h"
#include <math.h> 
#include <iostream>
#include "../Scene/Pivot.h"

using namespace std;

const int SAMPLING_RATE = 25;

BezierSurface::BezierSurface()
	: SceneObject(),
	m_num_control_row(0),
	m_num_control_col(0),
	m_control_points(NULL)
{
	m_layout.Push<float>(4);
	m_layout.Push<float>(3);
	m_layout.Push<float>(2);
	//PushDraggablePoints();
}
 
BezierSurface::BezierSurface(int rows, int cols, glm::vec4 ** control_points)
	: SceneObject(),
	m_num_control_row(rows),
	m_num_control_col(cols),
	m_control_points(control_points)
{
	m_layout.Push<float>(4);
	m_layout.Push<float>(3);
	m_layout.Push<float>(2);

	EvaluateBezierSurface();
	//PushDraggablePoints();
}

BezierSurface::BezierSurface(int rows, int cols, glm::vec4 ** control_points, MaterialTexture mat_tex)
	: SceneObject(glm::vec3(), glm::mat4(), mat_tex),
	m_num_control_row(rows),
	m_num_control_col(cols),
	m_control_points(control_points)
{
	m_layout.Push<float>(4);
	m_layout.Push<float>(3);
	m_layout.Push<float>(2);

	EvaluateBezierSurface();
	//PushDraggablePoints();
}

BezierSurface::~BezierSurface()
{
	for (int i = 0; i < m_num_control_row; i++)
		delete[] m_draggable_points[i];
	delete m_draggable_points;
}

glm::vec4 BezierSurface::CalculatePointOnBezierCurve(float t, glm::vec4* calculation_points, int len)
{
	glm::vec4 point;

	int p = len - 1;

	for (int i = 0; i < len; i++) {
		float k = (Factorial(p) / (Factorial(i) * Factorial(p - i))) * pow((1 - t), p - i) * pow((t), i);
		glm::vec4  newPoint = glm::vec4(calculation_points[i].x * k, calculation_points[i].y * k, calculation_points[i].z * k, 1);
		point = point + newPoint;
	}

	point.w = 1;
	return point;
}

glm::vec4 BezierSurface::CalculatePointOnBezierSurface(float u, float v, glm::vec4** calculation_points, int r, int c)
{
	glm::vec4* uCurve = new glm::vec4[r];

	for (int i = 0; i < r; i++) {
		glm::vec4* selected_cp = new glm::vec4[c];

		for (int j = 0; j < c; j++)
			selected_cp[j] = (calculation_points[i][j]);

		uCurve[i] = CalculatePointOnBezierCurve(u, selected_cp, c);
		delete[] selected_cp;
	}
	glm::vec4 point = CalculatePointOnBezierCurve(v, uCurve, r);
	delete[] uCurve;

	return point;
}

void BezierSurface::DragSelectedControlPoints(glm::vec3 drag)
{
	if (m_draggable_points == nullptr)
	{
		PushDraggablePoints();
	}

	for (int i = 0; i < m_num_control_row; i++)
	{
		for (int j = 0; j < m_num_control_col; j++)
		{
			//DraggablePoint cur = ;
			if (m_draggable_points[i][j].IsSelected())
			{
				m_draggable_points[i][j].Translate(drag * pow(m_control_point_scale, -1));
				m_control_points[i][j] = glm::vec4(m_draggable_points[i][j].GetOrigin(), 1.0f);
				//m_draggable_points[i][j].MoveTo(m_control_points[i][j]);
				EvaluateBezierSurface();
			}
		}
	}
}

void BezierSurface::EvaluateBezierSurface()
{
	m_vertices.clear();
	Vertex v;
	for (int i = 0; i < SAMPLING_RATE; i++)
		for (int j = 0; j < SAMPLING_RATE; j++) {
			v.SetPosition(CalculatePointOnBezierSurface(((float)i) / ((float)SAMPLING_RATE - 1.0f), ((float)j) / ((float)SAMPLING_RATE - 1.0f),
				m_control_points, m_num_control_row, m_num_control_col));
			v.SetTextureCoords(glm::vec2(((float)i) / ((float)SAMPLING_RATE - 1.0f), ((float)j) / ((float)SAMPLING_RATE - 1.0f)));
			v.SetNormal(CalculateNormal(((float)i) / ((float)SAMPLING_RATE), ((float)j) / ((float)SAMPLING_RATE)));
			m_vertices.push_back(v);
		}
	CalculateIndices();
}

void BezierSurface::DrawWireFrame(Renderer renderer, Shader* shader, VertexArray* vArray)
{
	VertexBuffer vBuffer = VertexBuffer(&GetVertices().at(0), sizeof(Vertex) * GetVertices().size());
	shader->Bind();
	vArray->AddBuffer(vBuffer, m_layout);

	m_mat_tex.tex.Unbind();
	shader->SetMaterial(m_mat_tex.mat.GetUniformName(), Material());
	shader->SetUniformMat4f(this->GetTransformName(), this->GetTransform());

	vector<unsigned int> indices = GetIndices();
	IndexBuffer iBuffer(&indices.at(0), 4);

	for (int i = 0; i < indices.size(); i += 4)
	{
		iBuffer.SetBufferData(&indices.at(i), 4);
		renderer.Draw(*vArray, iBuffer, *shader, GL_LINE_LOOP);
	}
}

void BezierSurface::DrawControlPoints(Renderer renderer, Shader * shader, VertexArray * vArray)
{
	if (m_draggable_points == nullptr)
	{
		PushDraggablePoints();
	}

		for (int i = 0; i < m_num_control_row; i++)
		{
			for (int j = 0; j < m_num_control_col; j++)
			{
				shader->SetUniformMat4f(this->GetTransformName(), m_draggable_points[i][j].GetTransform());
				m_draggable_points[i][j].Draw(renderer, shader, vArray);

				if (m_draggable_points[i][j].IsSelected())
				{
					m_piv.SceneObject::SetTransform(m_draggable_points[i][j].GetTransform());
					m_piv.Draw(renderer, shader, vArray);
				}
			}
		}
}

void BezierSurface::Draw(Renderer renderer, Shader* shader, VertexArray* vArray)
{

	VertexBuffer vBuffer = VertexBuffer(&GetVertices().at(0), sizeof(Vertex) * GetVertices().size());
	shader->Bind();
	vArray->AddBuffer(vBuffer, m_layout);

	m_mat_tex.tex.Bind();
	shader->SetUniform1i(m_mat_tex.tex.GetUniformName(), 0);
	shader->SetMaterial(m_mat_tex.mat.GetUniformName(), m_mat_tex.mat);
	shader->SetUniformMat4f(this->GetTransformName(), this->GetTransform());

	vector<unsigned int> indices = GetIndices();
	IndexBuffer iBuffer(&indices.at(0), 4);

	for (int i = 0; i < indices.size(); i += 4)
	{
		iBuffer.SetBufferData(&indices.at(i), 4);
		renderer.Draw(*vArray, iBuffer, *shader, GL_TRIANGLE_FAN);
	}

}

void BezierSurface::HitAndSelect(Ray ray)
{
	if (m_draggable_points == nullptr)
	{
		PushDraggablePoints();
	}
	for (int i = m_num_control_row - 1; i >= 0; i--)
	{
		for (int j = m_num_control_col -1; j >= 0; j--)
		{
			if (m_draggable_points[i][j].Hit(ray, m_control_point_scale))
			{
				m_draggable_points[i][j].SetMaterial(Material(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
					glm::vec4(), glm::vec4(), 10.0f, m_mat_tex.mat.GetUniformName()));
				m_draggable_points[i][j].SetSelected(true);

				//cout << "selected " << i << ", " << j << endl;
			}
			else
			{
				m_draggable_points[i][j].SetMaterial(Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
					glm::vec4(), glm::vec4(), 10.0f, m_mat_tex.mat.GetUniformName()));
				m_draggable_points[i][j].SetSelected(false);
			}

		}
	}
}

void BezierSurface::CalculateIndices()
{
	m_indices.clear();
	for (int j = 1; j <= SAMPLING_RATE * (SAMPLING_RATE - 1); j++) {

		if (!((j - 1) % SAMPLING_RATE == (SAMPLING_RATE - 1)))
		{
			m_indices.push_back(j - 1);
			m_indices.push_back(j);
			m_indices.push_back(SAMPLING_RATE + j);
			m_indices.push_back(SAMPLING_RATE - 1 + j);
		}
	}
}

void BezierSurface::PushDraggablePoints()
{
	m_draggable_points = new DraggablePoint*[m_num_control_row];
	for (int i = 0; i < m_num_control_row; i++)
	{
		m_draggable_points[i] = new DraggablePoint[m_num_control_col];
		for (int j = 0; j < m_num_control_col; j++)
		{
			DraggablePoint point = DraggablePoint();
			glm::mat4 m1 = point.GetTransform() * (this->GetTransform());
			point.SetTransform(m1);
			point.MoveTo(m_control_points[i][j]);
			point.Scale(glm::vec3(m_control_point_scale));
			m_draggable_points[i][j] = point;
		}
	}
}

glm::vec4 BezierSurface::CalculateDerivativeSum(vector<glm::vec4> v, float t, int direction)
{
	int degree;
	if (direction == 0)
		degree = m_num_control_row - 1;
	else
		degree = m_num_control_col - 1;

	glm::vec4 sum1;
	glm::vec4 sum2;

	for (int n = 0; n < degree; n++)
	{
		sum1 += (Factorial(degree) / (Factorial(degree - n) * Factorial(n)))
			* pow(t, n) * pow(1.0f - t, degree - n)
			* (v.at(n + 1));
	}

	for (int n = 0; n < degree; n++)
	{
		sum2 += (Factorial(degree) / (Factorial(degree - n) * Factorial(n)))
			* pow(t, n) * pow(1.0f - t, degree - n)
			* (v.at(n));
	}
	return (float)(degree + 1) * (sum1 - sum2);
}

glm::vec3 BezierSurface::CalculateNormal(float u, float v)
{
	vector<glm::vec4> p;
	vector<glm::vec4> vCurve;
	for (int j = 0; j < m_num_control_col; ++j)
	{
		for (int i = 0; i < m_num_control_row; ++i)
		{
			p.push_back(m_control_points[i][j]);
		}
		vCurve.push_back(CalculatePointOnBezierCurve(v, &p.at(0), p.size()));
		p.clear();
	}

	glm::vec3 dU = CalculateDerivativeSum(vCurve, u, 1);

	//-----------------------------------------------------------------------------------\\

	vector<glm::vec4> uCurve;
	for (int i = 0; i < m_num_control_row; ++i) {
		uCurve.push_back(CalculatePointOnBezierCurve(v, m_control_points[i], m_num_control_col));
	}

	glm::vec3 dV = CalculateDerivativeSum(uCurve, v, 0);

	return glm::normalize(glm::cross(dU, dV));
}