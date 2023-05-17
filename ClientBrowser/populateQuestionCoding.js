window.onload = function () {
  var form = document.getElementById("codingQuestionForm");

  var question = {
    type: "coding",
    text: "Write a function in JavaScript that reverses a string.",
  };

  var questionDiv = document.createElement("div");
  questionDiv.className = "question-module coding";

  var questionText = document.createElement("p");
  questionText.textContent = question.text;
  questionDiv.appendChild(questionText);

  if (question.type === "coding") {
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

    fetch("http://your-server-url.com/submit-answer", {
      method: "POST",
      body: formData,
    }).then(function (response) {
      if (response.ok) {
        alert("Answer submitted successfully!");
      } else {
        alert("Error: " + response.statusText);
        if (data.attempts === 3) {
          // If it was the third attempt, show the correct answer.
          var correctAnswerDiv = document.createElement("div");
          correctAnswerDiv.textContent = "Correct Answer: " + question.answer;
          form.appendChild(correctAnswerDiv);
        }
      }
    });
  };
};
