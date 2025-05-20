# TECHIN 515 – Lab 2: Quantization and Pruning Methods

## Objective

In this lab, you will explore model compression techniques to reduce the size and improve the efficiency of deep learning models. Specifically, you will apply three **post-training quantization** methods and one **pruning** technique to an EfficientNet-based model trained on the Cats vs. Dogs dataset.

By the end of this lab, you will be able to:
- Apply multiple TensorFlow Lite quantization methods
- Implement pruning using TensorFlow Model Optimization Toolkit
- Compare size and accuracy trade-offs across compression methods

---

## Setup Instructions

Before starting, make sure you have:
- Activated the `TECHIN515` virtual environment. The virtual environment is provided in `environment.yml`. Please follow the instructions in lecture or [here](https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html)
- Installed required packages using the provided `pip install` cells:
  - `tensorflow_datasets`
  - `tensorflow_model_optimization`

---

## Lab Tasks

### 1. Dataset Preparation
- Download and prepare the `cats_vs_dogs` dataset using TensorFlow Datasets.

### 2. Model Preparation
- Load and fine-tune an EfficientNet model (`efnet`) for binary classification (cat vs. dog).
- This model will serve as your baseline for comparison.

### 3. Post-Training Quantization

Apply the following quantization techniques using TensorFlow Lite:
- **Float-16 Quantization**  
  Reduce model precision to float16 for size reduction.
  
- **Dynamic Range Quantization**  
  Quantize weights to 8-bit integers; activations remain in float.

- **Integer Quantization**  
  Quantize both weights and activations using a representative dataset for calibration.

### 4. Pruning

- Apply **strip pruning** using `tensorflow_model_optimization`.
- Retrain the model while gradually pruning weights during training.
- Strip the pruning wrappers to create a final deployable model.

---

## Evaluation

For each compression method (quantization and pruning), compare:
- Model size on disk
- Inference accuracy
- Impact on performance vs. baseline

---

## Submission Requirements

Submit the following:
1. The completed Jupyter notebook with all cells executed
2. Your responses to all discussion questions.
3. A short written reflection (1–2 paragraphs) comparing the compression techniques:
   - Which method yielded the smallest model?
   - Which maintained the highest accuracy?
   - Which would you recommend for edge deployment, and why?
