window.onload = function () {
  var form = document.getElementById("codingQuestionForm");

  // Create a single question from the file stream
  var questionFile = `11\nprogramchallenge\npython\nwrite a function and return the total of two input number"\nInput:("5", "10")\nOutput:"15"`;
  var lines = questionFile.split("\n");

  var question = {
    id: lines[0],
    type: lines[1],
    language: lines[2],
    text: lines[3],
    input: lines[4],
    output: lines[5]
  };

  var questionDiv = document.createElement("div");
  questionDiv.className = "question-module coding";

  var questionText = document.createElement("p");
  questionText.textContent = question.text;
  questionDiv.appendChild(questionText);

  if (question.type === "programchallenge") {
    var answerDiv = document.createElement("div");
    answerDiv.className = "form-group";

    var label = document.createElement("label");
    label.for = "codingAnswer";
    label.textContent = "Answer:";
    answerDiv.appendChild(label);

    var textarea = document.createElement("textarea");
    textarea.id = "codingAnswer";
    textarea.name = "codingAnswer";
    textarea.rows = "4";
    textarea.required = true;
    answerDiv.appendChild(textarea);

    questionDiv.appendChild(answerDiv);
  }

  form.appendChild(questionDiv);

  form.onsubmit = function (event) {
    event.preventDefault();
    var formData = new FormData(form);
    formData.append("questionNumber", question.id);

    fetch("http://your-server-url.com/submit-answer", {
      method: "POST",
      body: formData,
    }).then(function (response) {
      if (response.ok) {
        alert("Answer submitted successfully!");
      } else {
        alert("Error: " + response.statusText);
      }
    });
  };
};
