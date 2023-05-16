// Get the next and previous buttons
var nextButton = document.querySelector(".next-question");
var prevButton = document.querySelector(".prev-question");


nextButton.addEventListener("click", function () {
  fetch("http:4125", {
    method: "GET",
  })
    .then(response => response.text())
    .then(data => {
      // Load the next question
      loadQuestionScript(data);
    })
    .catch(error => {
      console.error('Error:', error);
    });
});

prevButton.addEventListener("click", function () {
  fetch("http:4125", {
    method: "GET",
  })
    .then(response => response.text())
    .then(data => {
      // Load the previous question
      loadQuestionScript(data);
    })
    .catch(error => {
      console.error('Error:', error);
    });
});

function loadQuestionScript(questionData) {
  // Determine the type of question from the second line of the response
  var type = questionData.split("\n")[1];

  // Based on the type, choose the script to load
  var scriptFile = "";
  if (type === "mcq") {
    scriptFile = "populateQuestion.js";
  } else if (type === "programchallenge") {
    scriptFile = "populateQuestionCoding.js";
  } else {
    console.error('Unknown question type:', type);
    return;
  }
  window.questionData = questionData;

  // Create a script tag, set its source to the chosen file, and append it to the body of the document
  var scriptTag = document.createElement("script");
  scriptTag.src = scriptFile;
  document.body.appendChild(scriptTag);
}
